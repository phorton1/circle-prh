// std_kernel.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>

#include "std_kernel.h"
#include <circle/util.h>
#include <circle/types.h>
#include <circle/gpiopin.h>
#include <audio/AudioStream.h>

static const char log_name[] = "kernel";

#if USE_UGUI
	#include <audio/ui/app.h>
#endif


// memory usage (980M+ available on rPi3b)
//
// 0x00008000   Kernal program and BSS are limited to 2MB total.
// ....
// 0x00500000   Heap Start at 5M and allows almost all memory to be malloc'd
// 0x1F000000	Not using rpi_stub would get in the way
// 0x3b001000   Page allocator start at 988M and alllows for 4M of pages
// 0x3c000000   standard 64MB of GPU memory starts here (from top)
// 0x3F000000	16 MByte Peripherals
// 0x40000000	1GB address Local peripherals


extern void setup();
extern void loop();


CKernel::CKernel(void) :
	#if USE_SCREEN
		m_Screen(800, 480),	// m_Options.GetWidth(), m_Options.GetHeight()),
	#endif
	#if USE_MINI_SERIAL
		m_MiniUart(&m_Interrupt),
	#endif
	m_Timer(&m_Interrupt),
	#if USE_MAIN_SERIAL
		m_Serial(&m_Interrupt, TRUE),
	#endif
	m_Logger(LogDebug,&m_Timer)	// m_Options.GetLogLevel(), &m_Timer)
	#if USE_USB
		,m_DWHCI(&m_Interrupt, &m_Timer)
	#endif
	#if USE_UGUI
		,m_GUI(&m_Screen)
	#endif
{
	m_ActLED.Toggle();
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
		#if !USE_MINI_SERIAL && !USE_MAIN_SERIAL 
			if (bOK)
				bOK = m_Logger.Initialize(&m_Screen);
		#endif
	#endif

	#if USE_MINI_SERIAL
		if (bOK)
			bOK = m_MiniUart.Initialize(115200);
		if (bOK)
			bOK = m_Logger.Initialize(&m_MiniUart);
	#endif	

	if (bOK)
		bOK = m_Interrupt.Initialize();
	if (bOK)
		bOK = m_Timer.Initialize();

	#if USE_MAIN_SERIAL
		if (bOK)
			bOK = m_Serial.Initialize(115200);
		if (bOK)
			bOK = m_Logger.Initialize(&m_Serial);
	#endif	

	#if USE_USB
		if (bOK)
			bOK = m_DWHCI.Initialize ();
	#endif
	
	#if USE_UGUI
		if (bOK)
		{
			m_TouchScreen.Initialize ();
			bOK = m_GUI.Initialize ();
		}
	#endif
	
	return bOK;
}



u32 main_loop_counter = 0;


TShutdownMode CKernel::Run(void)
{
	LOG("std_kernel " __DATE__ " " __TIME__ " available memory=%d",mem_get_size());
	setup();
	printf("memory after setup()=%d\n",mem_get_size());
	
#if USE_UGUI
	CApplication app;
	printf("memory after app constructed()=%d\n",mem_get_size());
#endif
	
	m_Timer.MsDelay(500);
	printf("ready ...\n");
	
	#if 1
		CGPIOPin toTeensy(25,GPIOModeOutput);
		toTeensy.Write(1);
	#endif
	
	while (1)
	{
		#if USE_UGUI
			m_GUI.Update ();
		#endif
		
		loop();
		m_Scheduler.Yield();
		main_loop_counter++;
	}
	
	return ShutdownHalt;

}


