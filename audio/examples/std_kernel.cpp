// std_kernel.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>

#include "std_kernel.h"
#include <circle/util.h>
#include <circle/types.h>
#include <circle/gpiopin.h>
#include <audio/AudioStream.h>

#if USE_UGUI
	#include <audio/ui/scopewindow.h>
	#include <audio/ui/controlwindow.h>
#endif

#if 0 	// USE_SCREEN
	#include "statusScreen.h"
#endif


extern void setup();
extern void loop();


static const char log_name[] = "kernel";


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
	m_Logger(m_Options.GetLogLevel(), &m_Timer)
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
	
	printf("kernel intialize finished\n");
	
	return bOK;
}



u32 main_loop_counter = 0;


TShutdownMode CKernel::Run(void)
{
	LOG("std_kernel " __DATE__ " " __TIME__,0);

#if 0 // USE_SCREEN
	LOG("using statusScreen",0);
	print_screen("hello\n");
#endif

	setup();
	
#if 0 // USE_SCREEN
	statusScreen status(&m_Screen);
	status.init();
#endif	

#if USE_UGUI
	// CScopeConfig ui_Config;
	// ui_Config.AddParamSet(20, 44, 0);

	CScopeWindow ScopeWindow(0, 0); // , /* &m_Recorder,*/ &ui_Config);
	CControlWindow ControlWindow(600, 0, &ScopeWindow);	// , /* &m_Recorder,*/ &ui_Config);
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


