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

#define SEND_FAKE_MIDI	0
	// send fake midi messages to test esp32_PiLooper


static const char log_name[] = "kernel";

#define DEBUG_TOUCH 	0

#define START_COUNT   	5
static int start_count = 0;



CKernel::CKernel(void) :
	#if USE_SCREEN
		m_Screen(800, 480),
	#endif
	m_Timer(&m_Interrupt),
	m_Serial(&m_Interrupt, TRUE),
	m_Logger(LogDebug,&m_Timer)
	#if USE_ILI_DEVICE
		,m_SPI()
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
	#endif

	if (bOK)
		bOK = m_Interrupt.Initialize();
	if (bOK)
		bOK = m_Timer.Initialize();

	#if USE_SCREEN
		if (bOK)
			bOK = m_Logger.Initialize(&m_Screen);
	#endif

	if (bOK)
		bOK = m_Serial.Initialize(115200);

	#if !USE_SCREEN
		if (bOK)
			bOK = m_Logger.Initialize(&m_Serial);
	#endif

	#if USE_ILI_DEVICE
		if (bOK)
			bOK = m_SPI.Initialize();

		LOG("constructing ILI%d",USE_ILI_DEVICE);
		#if USE_ILI_DEVICE == 9488
			m_tft_device = new ILI9488(&m_SPI);
		#else
			m_tft_device = new ILI9846(&m_SPI);
		#endif
		LOG("ILI%d constructed",USE_ILI_DEVICE);
	#endif

	LOG("Kernel::Initialize() completed",0);

	return bOK;
}


#if USE_XPT2046
	static bool started_calibration = 0;
#endif


TShutdownMode CKernel::Run(void)
	// calls the CoreTask to do all the work
{
	LOG("tsTest " __DATE__ " " __TIME__,0);

	while (1)
	{
		if (start_count < START_COUNT)
		{
			m_ActLED.Toggle();

			start_count++;
			LOG("start_count(%d)",start_count);
			CTimer::Get()->MsDelay(1000);
			if (start_count == START_COUNT)
			{
				#if USE_XPT2046
					// By convention, all TFTs are constructed in rotation 0.
					// We can call m_tft_device->GetWidth() and GetHeight() right after
					// to get the raw physical width and height in rotation 0 in
					// order to construct the xpt2046.  HOWEVER, if tft initialization
					// sets a different rotation (as I typically like to do rotation(3)
					// during initialization), then, thereafter, GetWidth() and GetHeight()
					// are set to the LOGICAL width and height, and cannot be used as-is
					// for xpt2046 construction.

					LOG("constructing XPT2046",0);
					m_xpt2046 = new XPT2046(&m_SPI,m_tft_device);
					LOG("XPT2046 constructed",0);
				#endif

				#if USE_ILI_DEVICE
					LOG("initializing ILI%d",USE_ILI_DEVICE);
					m_tft_device->Initialize();
					LOG("ILI%d initialized",USE_ILI_DEVICE);
				#endif
				
				#if USE_XPT2046
					LOG("initializing XPT2046",0);
					m_xpt2046->setRotation(m_tft_device->getRotation());
					m_xpt2046->RegisterEventHandler(touchEventStub,this);
					LOG("XPT2046 initialized",0);
				#endif
			}
		}
		else
		{

			static unsigned last_tick = 0;
			unsigned tick = CTimer::Get()->GetTicks();
			if (tick - last_tick > 100)
			{
				last_tick = tick;
				m_ActLED.Toggle();
			}

			#if SEND_FAKE_MIDI
			    static uint8_t msg[4] = { 0x0b, 0xb0, 0x01, 0x01 };

				static unsigned last_midi = 0;
				if (tick - last_midi > 50)
				{
					last_midi = tick;
					if (msg[3] % 8 == 0)
						LOG("sending midi 0x%08x",*(uint32_t *) msg);
					m_Serial.Write(msg,4);
					msg[3] = (msg[3]+1 % 128);
				}
			#endif

			#if USE_XPT2046
				if (started_calibration && !m_xpt2046->inCalibration())
				{
					started_calibration = 0;
					m_tft_device->distinctivePattern();
				}

				m_xpt2046->Update();
				#if DEBUG_TOUCH
					CTimer::Get()->MsDelay(100);
				#else
					CTimer::Get()->MsDelay(10);
				#endif
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
		#if DEBUG_TOUCH
			LOG("touchEventHandler(%d,%d,%s)",
				x,y,
				(event == TouchScreenEventFingerDown) ? "down" :
				(event == TouchScreenEventFingerMove) ? "move" :
				(event == TouchScreenEventFingerUp)   ? "up" : "unknown");
		#endif

		#if 1
			// press and release the red box to advance rotation

			if (event == TouchScreenEventFingerUp &&
				x < 50 && y < 50)
			{
				u8 rot = m_tft_device->getRotation();
				rot = (rot + 1) % 4;
				LOG("SET_ROTATION(%d)",rot);
				m_tft_device->setRotation(rot);
				m_xpt2046->setRotation(rot);
				m_tft_device->distinctivePattern();
			}

			// press and release green box to do a calibration
			#if USE_XPT2046
				else if (event == TouchScreenEventFingerUp &&
					x > (m_tft_device->GetWidth()-50) && y < 50)
				{
					started_calibration = 1;
					m_xpt2046->startCalibration();
				}
			#endif

			else if (event == TouchScreenEventFingerDown ||
					 event == TouchScreenEventFingerMove)
			{
				for (int i=-1; i<=1; i++)
				{
					for (int j=-1; j<=1; j++)
					{
						m_tft_device->SetPixel(x+i,y+j,RGB565_YELLOW);
					}
				}
			}

	    #endif
	}

#endif
