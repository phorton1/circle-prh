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

#define DEBUG_TOUCH 	0

#define START_COUNT   	5
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
					m_xpt2046 = new XPT2046(
						&m_SPI,
						m_tft_device->GetWidth(),
						m_tft_device->GetHeight());
					LOG("XPT2046 constructed",0);
				#endif

				LOG("initializing ILI%d",USE_ILI_DEVICE);
				m_tft_device->Initialize();
				LOG("ILI%d initialized",USE_ILI_DEVICE);

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
			#if USE_XPT2046
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

	// some RGB565 colors for testing

	#define C_BLACK     0x0000
	#define C_BLUE      0x001F
	#define C_GREEN     0x07E0
	#define C_RED       0xF800
	#define C_YELLOW    0xFFE0
	#define C_CYAN      0x07FF
	#define C_WHITE     0xFFFF



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

				u16 width = m_tft_device->GetWidth();
				u16 height = m_tft_device->GetHeight();

				m_tft_device->fillRect(0,0,width-1,height-1,C_BLACK);
				m_tft_device->fillRect(width/3,height/3,2*width/3-1,2*height/3-1,C_CYAN);
				m_tft_device->fillRect(0,0,20,20,C_RED);
				m_tft_device->fillRect(width-1-30,0,width-1,30,C_GREEN);
				m_tft_device->fillRect(0,height-1-40,40,height-1,C_BLUE);
				m_tft_device->fillRect(width-1-50,height-1-50,width-1,height-1,C_WHITE);
			}
			else if (event == TouchScreenEventFingerDown ||
					 event == TouchScreenEventFingerMove)
			{
				m_tft_device->SetPixel(x,y,C_YELLOW);
			}

	    #endif
	}

#endif
