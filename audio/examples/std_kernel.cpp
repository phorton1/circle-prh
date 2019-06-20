// std_kernel.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>

#include "std_kernel.h"
#include <circle/util.h>
#include <circle/types.h>
#include <circle/gpiopin.h>

#if USE_SCREEN
	#include "statusScreen.h"
	#define STATUS_TIME  		5
#endif


extern void setup();
extern void loop();


static const char log_name[] = "kernel";


CKernel::CKernel(void) :
	#if USE_SCREEN
		m_Screen(m_Options.GetWidth(), m_Options.GetHeight()),
	#endif
	#if USE_MINI_SERIAL
		m_MiniUart(&m_Interrupt),
	#endif
	m_Timer(&m_Interrupt),
	#if USE_MAIN_SERIAL
		m_Serial(&m_Interrupt, TRUE),
	#endif
	m_Logger(m_Options.GetLogLevel(), &m_Timer)
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

	printf("kernel intialize finished\n");
	
	return bOK;
}



u32 main_loop_counter = 0;


TShutdownMode CKernel::Run(void)
{
	LOG("std_kernel " __DATE__ " " __TIME__,0);

#if USE_SCREEN
	LOG("using statusScreen",0);
	print_screen("hello\n");
#endif

	setup();
	
#if USE_SCREEN
	statusScreen status(&m_Screen);
	u32 status_time = 0;
#endif	

	m_Timer.MsDelay(500);
	printf("ready ...\n");
	
	#if 1
		CGPIOPin toTeensy(25,GPIOModeOutput);
		toTeensy.Write(1);
	#endif
	
	while (1)
	{
		loop();
		m_Scheduler.Yield();
		main_loop_counter++;
		
		// NOTE: CTimer::GetTicks() is 100ths of a second !!
		
		#if 1 //  USE_SCREEN
		
			// On the verge of understanding.
			// hdmi squiggly line wm8731 master bug
			//		
			// There MUST be a delay here of at least 1 MS.
			//
			// Cannot call getTicks() in a tight loop or
			// else the HDMI and/or audio get messed up.
			
			CTimer::Get()->usDelay(1000);
			
			// It is not the STATUS_TIME, per se, which is plenty
			// long. It's that we're checking the time a million
			// times a second, or more, in a tight loop ....
			// There are no tasks() so yield does nothing.
			// loop() has no body.  It has something to do
			// with interrupts, though GetTicks() is just an
			// integer accessor ... it doesn't "do" anything.
	
			int i = CTimer::Get()->GetTicks();
			if (i > status_time + STATUS_TIME)
			{
				status.update();
				status_time =  CTimer::Get()->GetTicks();			
			}
		#endif		
	}
	
	return ShutdownHalt;

}


