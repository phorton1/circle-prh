// kernel.cpp
//
// Based on Circle - A C++ bare metal environment for Raspberry Pi
// which is Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// This code is Copyright (C) 2019, Patrick Horton, no rights reserved.
//
// The construction of the ILI and XPT2046 devices is deferred until
// CKernel::Initialize() so that LOG() can be used to debug them, and
// their initialize methods are put off until CKernel::run() to allow
// for a five second idle loop for starting a logic analyizer after
// construction.
//
// In deployment mode, the CTOR debugging would be turned off and
// static construction would be done.

#include "kernel.h"
#include <circle/util.h>
#include <circle/types.h>

static const char log_name[] = "kernel";

#define START_COUNT   5
static int start_count = 0;



CKernel::CKernel(void) :
	#if USE_SCREEN
		m_Screen(800, 480),
	#endif
	m_Timer(&m_Interrupt),
	#if !USE_SCREEN
		m_Serial(&m_Interrupt, TRUE),
	#endif
	m_Logger(LogDebug,&m_Timer),
	m_SPI()
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

	#if USE_SCREEN
		if (bOK)
			bOK = m_Logger.Initialize(&m_Screen);
	#else
		if (bOK)
			bOK = m_Serial.Initialize(115200);
		if (bOK)
			bOK = m_Logger.Initialize(&m_Serial);
	#endif

	if (bOK)
		bOK = m_SPI.Initialize();

				LOG("constructing ILI%d",USE_ILI_DEVICE);
				#if USE_ILI_DEVICE == 9488
					m_tft_device = new ILI9488(&m_SPI);
				#else
					m_tft_device = new ILI9846(&m_SPI);
				#endif
				LOG("ILI%d constructed",USE_ILI_DEVICE);

	return bOK;
}



TShutdownMode CKernel::Run(void)
	// calls the CoreTask to do all the work
{
	LOG("tsTest " __DATE__ " " __TIME__,0);

	while (1)
	{
		m_ActLED.Toggle();

		if (start_count < START_COUNT)
		{
			start_count++;
			LOG("start_count(%d)",start_count);
			CTimer::Get()->MsDelay(1000);
			if (start_count == START_COUNT)
			{
				// LOG("constructing ILI%d",USE_ILI_DEVICE);
				// #if USE_ILI_DEVICE == 9488
				// 	m_tft_device = new ILI9488(&m_SPI);
				// #else
				// 	m_tft_device = new ILI9846(&m_SPI);
				// #endif
				// LOG("ILI%d constructed",USE_ILI_DEVICE);


				#if USE_XPT2046
					LOG("constructing XPT2046",0);
					m_xpt2046 = new XPT2046(&m_SPI);
					LOG("XPT2046 constructed",0);
				#endif

				LOG("initializing ILI%d",USE_ILI_DEVICE);
				m_tft_device->Initialize();
				LOG("ILI%d initialized",USE_ILI_DEVICE);

				#if USE_XPT2046
					LOG("initializing XPT2046",0);
					m_xpt2046->setDimensions(m_tft_device->GetWidth(),m_tft_device->GetHeight());
					m_xpt2046->setRotation(m_tft_device->getRotation());
					m_xpt2046->RegisterEventHandler(touchEventStub,this);
					LOG("XPT2046 initialized",0);
				#endif
			}
		}
		else
		{
			#if USE_XPT2046
				m_xpt2046->Update();
				CTimer::Get()->MsDelay(100);
			#else
				CTimer::Get()->MsDelay(1000);
			#endif
		}
	}

	return ShutdownHalt;
}


//--------------------------
// touch event handler
//--------------------------

#if USE_XPT2046

	void CKernel::touchEventStub(
		void *pThis,
		TTouchScreenEvent event,
		unsigned id,
		unsigned x,
		unsigned y)
	{
		assert(pThis);
		((CKernel *)pThis)->touchEventHandler(event,id,x,y);
	}


	void CKernel::touchEventHandler(
		TTouchScreenEvent event,
		unsigned id,
		unsigned x,
		unsigned y)
	{
		LOG("touchEventHandler(%d,%d,%s)",
			x,y,
			(event == TouchScreenEventFingerDown) ? "down" :
			(event == TouchScreenEventFingerMove) ? "move" :
			(event == TouchScreenEventFingerUp)   ? "up" : "unknown");
	}

#endif
