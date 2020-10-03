#ifndef _kernel_h
#define _kernel_h

#define WITH_SCREEN   0

#include <circle/memory.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#if WITH_SCREEN
	#include <circle/screen.h>
#endif
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/serial.h>
#include <circle/types.h>
#include <circle/stdarg.h>
#include "hw.h"


void printf(const char *pMessage, ...);
	

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

	static CKernel *Get()  { return global_kernel; }
	CDevice *getOutputDevice()  { return &m_Serial; }

	
// private:
	
	static CKernel *global_kernel;
	void usbStuff(int what);

	// do not change this order
	CMemorySystem		m_Memory;
	CActLED				m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
#if WITH_SCREEN
	CScreenDevice		m_Screen;
#endif
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer				m_Timer;
	CSerialDevice 		m_Serial;
	CLogger				m_Logger;
};


#endif
