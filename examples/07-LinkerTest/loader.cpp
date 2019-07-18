// loader.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// Circle Squared - Copyright (C) 2019 Patrick Horton


#include "loader.h"
#include "kernel.h"
#include <circle/alloc.h>
#include <circle/memory.h>
#include <circle/synchronize.h>
#include <circle/util.h>


#define log_name "loader"

CLoader::~CLoader()
{
	init();
	m_pFS = 0;
}

CLoader::CLoader(FATFS *fs) :
	m_pFS(fs),
	m_size(0),
	m_buffer(0),
	m_entry_point(0)
{
}

void CLoader::init()
{
	// if (m_buffer)
	// 	free(m_buffer);
	m_size = 0;
	m_buffer = 0;
	m_entry_point = 0;
}

#define SAFETY 0x8000
#define PROGRAM_LOCATION 0x80000
	// 0x100000 does not work
	// 0x80000 works
	// (end + SAFETY) works
	// 0x108000	does not work
	// malloc(m_size); does not work

u8 CLoader::loadProgram(const char *filename)
	// Could not simply execute programs in memory gotten with alloc()/
	// I think that's because the kernel.ld linker script marks only
	// certain segments as executable, but I'mnot sure.
	//
	// It works down in the kernel memory space between 0x8000 and 0x200000
	//
	// We use an arbitrary location of 0x108000 (1 meg above the kernel)
	// as the location to load the executable.
{

	// debugging
	
	extern u32 _end;
	u32 end = (u32) &_end;
	u32 top = 0x200000;
	u32 avail = top - end;
	LOG("loadProgram(%s) kernel=0x8000  end=0x%X  top=0x%X  avail=%d bytes",filename,end,top,avail);
	
	// get the size of the file
	
	FILINFO finfo;
	if (FR_OK != f_stat(filename, &finfo))
	{
		LOG_ERROR("Could not stat %s",filename);
		return LOAD_NO_FILE;
	}
	LOG("file_size=%d",finfo.fsize);
	m_size = finfo.fsize;

	// open the file
	
	FIL file;
	if (FR_OK != f_open(&file, filename, FA_READ | FA_OPEN_EXISTING))
	{
		init();
		LOG_ERROR("Could not open %s",filename);
		return LOAD_NO_OPEN;
	}
	LOG("%s opened",filename);
	
	// allocate the memory
	
	m_buffer = (u8 *) PROGRAM_LOCATION;
	assert(m_buffer);
		
	// read it into memorry

	u32 got;
	if (FR_OK != f_read(&file, m_buffer, m_size, &got))
	{
		init();
		f_close(&file);
		LOG_ERROR("Could not read %s",filename);
		return LOAD_NO_READ;
	}
	if (got != m_size)
	{
		init();
		f_close(&file);
		LOG_ERROR("Error read %s expected(%d) got(%d)",filename,m_size,got);
		return LOAD_BAD_READ;
	}
	f_close(&file);
	
	// finished

	LOG("%s in memory at 0x%08x",filename,(u32)m_buffer);
	// display_bytes("program",m_buffer,m_size);
	return LOAD_SUCCESS;
}



int CLoader::callProgram()
{
	int rslt = 0;
	
#if 0
	
	typedef int (*fxn)();
	rslt = ((fxn)m_buffer)();
	return rslt;

#else	

	u32	addr = (u32)m_buffer;
	
	u32 pc;
	u32 sp;
	u32 lr;
	
	asm volatile
	(
		"mov %0,pc\n"
		"mov %1,sp\n"
		"mov %2,lr\n"
		: "=r" (pc),"=r" (sp),"=r" (lr)
	);
	
	printf("PC(%08x) SP(%08x) LR(%08x)\n",pc,sp,lr);
	
	if (0)
	{
		printf("stack:\n");
		u32 *p = (u32 *)sp;
		for (int i=0; i<32; i++)
		{
			printf("    %08x:   0x%08x\n",(u32)p,*p);
			p++;
		}
	}
	
	// display_bytes("relocated",(u8*)addr,8);
	LOG("calling method at %08x",addr);

	// InvalidateDataCache ();
	// InvalidateInstructionCache ();
	// FlushBranchTargetCache ();
	// asm volatile ("mcr p15, 0, %0, c8, c7,  0" : : "r" (0));	// invalidate unified TLB
	// DataSyncBarrier ();
	// FlushPrefetchBuffer ();
	
	asm volatile
	(
		"push {lr}\n"
		"mov r0,%1\n"
		"blx r0\n"
		"mov %0,r0\n"
		"pop {lr}\n"
		: "=r" (rslt)
		: "r" (addr)	// testFunc)
	);
	
	LOG("method returned 0x%08x",rslt);

	return rslt;
#endif

}



