// std_kernel.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>

#include "std_kernel.h"
#include <circle/util.h>
#include <circle/types.h>
#include <circle/gpiopin.h>

#if USE_SCREEN
	#include "statusScreen.h"
	#define STATUS_TIME  		20
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

		#if USE_SCREEN
			u32 now = m_Timer.GetTicks();
			if (now > status_time + STATUS_TIME)
			{
				status.update();
				status_time = now;			
			}
		#endif		
	}
	
	return ShutdownHalt;

}


