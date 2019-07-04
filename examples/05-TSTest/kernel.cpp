// kernel.cpp
//
// Based on Circle - A C++ bare metal environment for Raspberry Pi
// which is Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// This code is Copyright (C) 2019, Patrick Horton, no rights reserved.

#include "kernel.h"
#include <circle/util.h>
#include <circle/types.h>

static const char log_name[] = "kernel";


CKernel::CKernel(void) :
	#if USE_SCREEN
		m_Screen(800, 480),
	#endif
	m_Timer(&m_Interrupt),
	m_Serial(&m_Interrupt, TRUE),
	m_Logger(LogDebug,&m_Timer),
	m_SPI(),
	m_ili9486(&m_SPI),
	m_xpt2046(&m_SPI)
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
	#endif

	if (bOK)
		bOK = m_Interrupt.Initialize();
	if (bOK)
		bOK = m_Timer.Initialize();
	if (bOK)
		bOK = m_Serial.Initialize(115200);
	if (bOK)
		bOK = m_Logger.Initialize(&m_Serial);
	if (bOK)
		bOK = m_SPI.Initialize();
	if (bOK)
	{
		bOK = m_ili9486.Initialize();
		m_xpt2046.setDimensions(m_ili9486.getWidth(),m_ili9486.getHeight());
		m_xpt2046.setRotation(m_ili9486.getRotation());
	}
	
	return bOK;
}



TShutdownMode CKernel::Run(void)
	// calls the CoreTask to do all the work
{
	LOG("tsTest " __DATE__ " " __TIME__,0);
	
	while (1)
	{
		m_xpt2046.Update();
		CTimer::Get()->MsDelay(100);

	}

	return ShutdownHalt;
}


