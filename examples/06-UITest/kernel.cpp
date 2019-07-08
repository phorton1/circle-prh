// kernel.cpp
//
// Based on Circle - A C++ bare metal environment for Raspberry Pi
// which is Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// This code is Copyright (C) 2019, Patrick Horton, no rights reserved.

#include "kernel.h"
#include <circle/util.h>
#include <circle/types.h>
#include <ws/wsWindow.h>


static const char log_name[] = "kernel";


CKernel::CKernel(void) :
	m_Screen(800,480),		// 1440, 768),	// 800, 480),
	m_Timer(&m_Interrupt),
	m_Serial(&m_Interrupt, TRUE),
	m_Logger(LogDebug,&m_Timer)
	#if USE_USB
		,m_DWHCI(&m_Interrupt, &m_Timer)
	#endif
	#if USE_ALT_SCREEN
		,m_ili9486(&m_SPI)
		,m_xpt2046(&m_SPI)
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
	
	if (bOK)
		bOK = m_Screen.Initialize();
	if (bOK)
		bOK = m_Interrupt.Initialize();
	if (bOK)
		bOK = m_Timer.Initialize();
	if (bOK)
		bOK = m_Serial.Initialize(115200);
	if (bOK)
		bOK = m_Logger.Initialize(&m_Serial);
	#if USE_USB
		if (bOK)
			bOK = m_DWHCI.Initialize ();
	#endif

	if (bOK)
	{
		#if USE_ALT_SCREEN
			m_SPI.Initialize();
			m_ili9486.Initialize();
		#else
			m_TouchScreen.Initialize ();
		#endif
	}
	
	return bOK;
}



TShutdownMode CKernel::Run(void)
	// calls the CoreTask to do all the work
{
	LOG("06-UITest " __DATE__ " " __TIME__,0);

	CMouseDevice *pMouse = (CMouseDevice *) CDeviceNameService::Get ()->GetDevice ("mouse1", FALSE);
	
	#if USE_ALT_SCREEN
		CScreenDeviceBase *pUseScreen = &m_ili9486;
		CTouchScreenBase  *pTouch = &m_xpt2046;
	#else
		CScreenDeviceBase *pUseScreen = &m_Screen;
		CTouchScreenBase  *pTouch = (CTouchScreenBase *) CDeviceNameService::Get ()->GetDevice ("touch1", FALSE);
	#endif
	
	m_app.Initialize(pUseScreen,pTouch,pMouse);
	
	while (1)
	{
		m_app.timeSlice();
		CTimer::Get()->MsDelay(1);
	}

	return ShutdownHalt;
}


