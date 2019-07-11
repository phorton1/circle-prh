// kernel.h
//
// Based on Circle - A C++ bare metal environment for Raspberry Pi
// which is Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// This code is Copyright (C) 2019, Patrick Horton, no rights reserved.

#ifndef _kernel_h
#define _kernel_h

#define USE_USB         1
#define USE_ALT_SCREEN	0


#include <circle/memory.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/serial.h>
#include <circle/logger.h>

#if USE_ALT_SCREEN
	#include <devices/ili9486.h>
	#include <devices/xpt2046.h>
#else
	#include <circle/input/touchscreen.h>
#endif

#if USE_USB
	#include <circle/usb/dwhcidevice.h>
#endif

#include <ws/ws.h>



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
	CActLED				m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	CScreenDevice		m_Screen;
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer				m_Timer;
	CSerialDevice		m_Serial;
	CLogger				m_Logger;
	
	#if USE_USB
		CDWHCIDevice	m_DWHCI;
	#endif
	
	#if USE_ALT_SCREEN
		CSPIMaster	m_SPI;
		ILI9846 	m_ili9486;
		XPT2046 	m_xpt2046;
	#else	
		CTouchScreenDevice	m_TouchScreen;
	#endif
	
	wsApplication 	m_app;
	
};


#endif	// kernel_h
