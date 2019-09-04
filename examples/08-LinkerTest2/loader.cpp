// loader.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// Circle Squared - Copyright (C) 2019 Patrick Horton


#include "loader.h"
#include <system/std_kernel.h>
#include <circle/alloc.h>
#include <circle/memory.h>
#include <circle/synchronize.h>
#include <circle/util.h>


#define log_name "loader"

#define LOAD_LOCATION 0xc0000


CLoader::~CLoader()
{
	m_pFS = 0;
}

CLoader::CLoader(FATFS *fs) :
	m_pFS(fs)
{
	extern u32 _end;
	u32 end = (u32) &_end;			// end of BSS from linker
	
	m_location = LOAD_LOCATION;  	// arbitrary pre-determined load location
	m_unload_location = 0;
	
	u32 top = 0x200000;				// theoretical top of kernel program memory
	
	u32 avail = top - m_location;
	LOG("CLoader kernel=0x8000 end=0x%X top=0x%X location=0x%X  avail=%d",end,top,m_location,avail);
}


void CLoader::unloadProgram()
{
	if (m_unload_location)
	{
		LOG("unloading previous program at 0x%X",m_unload_location);
		typedef void (*fxn)();
		((fxn)m_unload_location)();
		m_unload_location = 0;
		LOG("back from program unload",0);
	}
}


int CLoader::loadProgram(const char *filename)
{
	LOG("loadProgram(%s)",filename);
	unloadProgram();
	
	if (m_unload_location)
	{
		LOG("unloading previous program at 0x%X",m_unload_location);
		typedef void (*fxn)();
		((fxn)m_unload_location)();
		m_unload_location = 0;
		LOG("back from program unload",0);
	}
	
	// get the size of the file
	
	FILINFO finfo;
	if (FR_OK != f_stat(filename, &finfo))
	{
		LOG_ERROR("Could not stat %s",filename);
		return 0;
	}
	LOG("file_size=%d",finfo.fsize);
	u32 size = finfo.fsize;

	// open the file
	
	FIL file;
	if (FR_OK != f_open(&file, filename, FA_READ | FA_OPEN_EXISTING))
	{
		LOG_ERROR("Could not open %s",filename);
		return 0;
	}
	LOG("%s opened",filename);
	
	// read it into memorry

	u32 got;
	u8 *buffer = (u8 *) m_location;
	if (FR_OK != f_read(&file, buffer, size, &got))
	{
		f_close(&file);
		LOG_ERROR("Could not read %s",filename);
		return 0;
	}
	if (got != size)
	{
		f_close(&file);
		LOG_ERROR("Error read %s expected(%d) got(%d)",filename,size,got);
		return 0;
	}
	f_close(&file);
	
	// call it

	LOG("calling %s in memory at 0x%08x",filename,m_location);
	typedef u32 (*fxn)();
	
	// don't know exactly what of the following magic calls
	// caused it to start working, but before this chunk of
	// code was added, the windows program would hang every
	// so often and it seemed to depend on waiting a bit before
	// pressing the button.  With the below magic code, I seem
	// to be able to run two applications at will.
	
	InvalidateDataCache ();
	InvalidateInstructionCache ();
	FlushBranchTargetCache ();
	// asm volatile ("mcr p15, 0, %0, c8, c7,  0" : : "r" (0));	// invalidate unified TLB
	DataSyncBarrier ();
	FlushPrefetchBuffer ();
	
	m_unload_location = ((fxn)m_location)();
	LOG("method returned 0x%08x",m_unload_location);
	return m_unload_location;
}


