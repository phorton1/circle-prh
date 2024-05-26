// kernel.h
//
// this test program uses it's own kernel
//
// Based on Circle - A C++ bare metal environment for Raspberry Pi
// which is Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// This code is Copyright (C) 2019, Patrick Horton, no rights reserved.

#ifndef _kernel_h
#define _kernel_h

#define USE_SCREEN  	 0

#define USE_ILI_DEVICE		9488	// 9488
#define USE_XPT2046			1	//	1


#include <circle/memory.h>
#include <utils/myActLED.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#if USE_SCREEN
	#include <circle/screen.h>
#endif
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>

#if USE_ILI_DEVICE == 9488
	#include <devices/ili9488.h>
#elif USE_ILI_DEVICE == 9486
	#include <devices/ili9486.h>
#endif

#if USE_XPT2046
	#include <devices/xpt2046.h>
#endif


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

	#if USE_ILI_DEVICE
		ILISPI_CLASS	    m_SPI;
	#endif

	#if USE_ILI_DEVICE == 9488
		ILI9488    		*m_tft_device;
	#elif USE_ILI_DEVICE == 9486
		ILI9846    		*m_tft_device;
	#endif

	#if USE_XPT2046
		XPT2046    		*m_xpt2046;

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
	#endif
};


#endif	// kernel_h
