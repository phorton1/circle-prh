// kernel.h
//
// this test program uses it's own kernel
//
// Based on Circle - A C++ bare metal environment for Raspberry Pi
// which is Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// This code is Copyright (C) 2019, Patrick Horton, no rights reserved.

#ifndef _kernel_h
#define _kernel_h

#define USE_SCREEN  	 1


#include <circle/memory.h>
#include <utils/myActLED.h>
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
#include <devices/ili9486.h>
#include <devices/xpt2046.h>
#include <utils/myUtils.h>


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
	
	CSPIMaster	        m_SPI;
	ILI9846    			m_ili9486;
	XPT2046    			m_xpt2046;
	
	void touchEventHandler(
		TTouchScreenEvent event,
		unsigned id,
		unsigned x,
		unsigned y);
	static void touchEventStub(
		void *pThis,
		TTouchScreenEvent event,
		unsigned id,
		unsigned x,
		unsigned y);
	
};


#endif	// kernel_h
