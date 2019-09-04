// kernel.h
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// Circle Squared - Copyright (C) 2019 Patrick Horton

#ifndef _kernel_h
#define _kernel_h

#define USE_SCREEN  	 0

#include <circle/memory.h>
#include <utils/myActLED.h>
#include <utils/myUtils.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#if USE_SCREEN
	#include <circle/screen.h>
#endif
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/serial.h>
#include <circle/logger.h>
#include <SDCard/emmc.h>
#include <fatfs/ff.h>
#include "loader.h"


enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

class CKernel
{
public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);

	TShutdownMode Run (void);

private:

	CMemorySystem		m_Memory;
	myActLED			m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	#if USE_SCREEN
		CScreenDevice	m_Screen;
	#endif
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer				m_Timer;
	CSerialDevice		m_Serial;
	CLogger				m_Logger;
	CEMMCDevice			m_EMMC;
	FATFS				m_FileSystem;
	CLoader				m_loader;
	
	void initFileSystem();
	
};

#endif
