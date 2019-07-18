// kernel.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// Circle Squared - Copyright (C) 2019 Patrick Horton
//
// LinkerTest - uses serial commands to load a shared library


#include "kernel.h"
#include <circle/util.h>
#include <circle/types.h>
#include <circle/gpiopin.h>

#define log_name "kernel"

#define SHOW_ROOT_DIRECTORY  1
#define DRIVE		"SD:"



CKernel::CKernel(void) :
	#if USE_SCREEN
		m_Screen(m_Options.GetWidth(), m_Options.GetHeight()),
	#endif
	m_Timer(&m_Interrupt),
	m_Serial(&m_Interrupt, TRUE),
	m_Logger(m_Options.GetLogLevel(), &m_Timer),
	m_EMMC(&m_Interrupt, &m_Timer, &m_ActLED),
	m_loader(&m_FileSystem)
{
	m_ActLED.Blink(5);
}


CKernel::~CKernel (void)
{
}



boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;
	
	#if USE_SCREEN
		if (bOK)
			bOK = m_Screen.Initialize();
	#endif

	if (bOK)
		bOK = m_Interrupt.Initialize();
	if (bOK)
		bOK = m_Timer.Initialize();
	if (bOK)
		bOK = m_Serial.Initialize(115200);
	if (bOK)
		bOK = m_Logger.Initialize(&m_Serial);
	if (bOK)
		bOK = m_EMMC.Initialize();

	return bOK;
}



void CKernel::initFileSystem()
{
	if (f_mount(&m_FileSystem, DRIVE, 1) != FR_OK)
	{
		LOG_ERROR("Cannot mount drive: %s", DRIVE);
		return;
	}

	#if SHOW_ROOT_DIRECTORY
		LOG("Contents of SD card",0);
		DIR Directory;
		FILINFO FileInfo;
		FRESULT Result = f_findfirst (&Directory, &FileInfo, DRIVE "/", "*");
		for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
		{
			if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
			{
				LOG("%-22s %ld", FileInfo.fname, FileInfo.fsize);
			}
			Result = f_findnext (&Directory, &FileInfo);
		}
	#endif
}



TShutdownMode CKernel::Run(void)
{
	m_Logger.Write(log_name, LogNotice, "LinkerTest " __DATE__ " " __TIME__);
	initFileSystem();
	
	m_Timer.MsDelay(400);
	printf("ready ...\n");
	CGPIOPin toTeensy(25,GPIOModeOutput);
	toTeensy.Write(1);

	while (1)
	{
		unsigned int c = 0;
		if (m_Serial.Read(&c,1))
		{
			c &= 0xff;
			if (c == 'a')
			{
				printf("Loading dynamic link library\n");
				if (LOAD_SUCCESS == m_loader.loadProgram("program.img"))
				{
					LOG("Calling entry point",0);
					int i = m_loader.callProgram();
					LOG("entry point returned 0x%08x",i);
				}
			}
		}

		#define BLINK_TIME 40	
		static u32 blink_time = 0;
		u32 now = m_Timer.GetTicks();
		if (now > blink_time + BLINK_TIME)
		{
			m_ActLED.Toggle();
			blink_time = now;
		}
	}	
	
	return ShutdownHalt;

}	// CKernel::Run()


