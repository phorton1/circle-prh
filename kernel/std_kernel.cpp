// std_kernel.cpp
//
// Based on Circle - A C++ bare metal environment for Raspberry Pi
// which is Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
//
// This code is Copyright (C) 2019, Patrick Horton, no rights reserved.


#include "std_kernel.h"
#include <circle/util.h>
#include <circle/types.h>
#include <circle/gpiopin.h>
#include <audio/AudioStream.h>
#if USE_UGUI
	#include <ui/app.h>
#endif

#define USE_GPIO_READY_PIN    0		// 25

static const char log_name[] = "kernel";


// memory usage (980M+ available on rPi3b)
// 0x00008000   Kernal program and BSS are limited to 2MB total.
//     ...      limited to 2MB!!
// 0x00500000   Heap Start at 5M and allows almost all memory to be malloc'd
//     ...      986+ MB Available
// 0x1F000000	Not using rpi_stub would get in the way
// 0x3b001000   Page allocator start at 988M and alllows for 4M of pages
//     ...      4MB available
// 0x3c000000   standard 64MB of GPU memory starts here (from top)
// 0x3F000000	16 MByte Peripherals
// 0x40000000	1GB address Local peripherals


extern void setup();
extern void loop();

u32 main_loop_counter = 0;


//---------------------------------------------
// CoreTask
//---------------------------------------------

CCoreTask *CCoreTask::s_pCoreTask = 0;


CCoreTask::CCoreTask(CKernel *pKernel)	:
	#ifdef USE_MULTI_CORE
		CMultiCoreSupport(&pKernel->m_Memory),
	#endif
	m_pKernel(pKernel),
	m_bAudioStarted(0),
	m_bUIStarted(0)
{
	s_pCoreTask = this;
}


CCoreTask::~CCoreTask()
{
	m_pKernel = 0;
}


void CCoreTask::runAudioSystem(unsigned nCore, bool init)
{
	if (init)
	{
		LOG("AudioSystem starting on Core(%d) mem=%dM",nCore,mem_get_size()/1000000);
		setup();
		m_bAudioStarted = 1;
		LOG("after AudioSystem started mem=%dM",mem_get_size()/1000000);
	}
	else
	{
		loop();
	}
}



#if USE_UGUI
	void CCoreTask::runUISystem(unsigned nCore, bool init)
	{
		if (init)
		{
			while (!m_bAudioStarted);
			
			LOG("UI starting on Core(%d) mem=%dM",nCore,mem_get_size()/1000000);
			CApplication *app = new CApplication();
			if (!app)
			{
				LOG_ERROR("Could not create CApplication",0);
			}
			else
			{
				LOG("after UI initialization mem=%dM",mem_get_size()/1000000);
				m_bUIStarted = 1;
			}
		}
		else if (m_bAudioStarted)
		{
			m_pKernel->m_GUI.Update ();
		}
	}
#endif



void CCoreTask::Run(unsigned nCore)
{
	LOG("Core(%d) starting ... mem=%dM",nCore,mem_get_size()/1000000);
	bool bCore0StartupReported = 0;
		
	// initialize the audio system on the given core
	
	if (nCore == CORE_FOR_AUDIO_SYSTEM)
		runAudioSystem(nCore,true);
	
	// initialilze the ui on the given core
	
	#if USE_UGUI
		if (nCore == CORE_FOR_UI_SYSTEM)
			runUISystem(nCore,true);
	#endif
	
	delay(5000);

	while (1)
	{
		// do a timeslice of the audio system on given core
		// if it's not core 0, this will just call Arduion loop()
		// method very rapidly, as audio will be handled by
		// inter-processor interrupts

		if (nCore == CORE_FOR_AUDIO_SYSTEM)
			runAudioSystem(nCore,false);
		
		// do a timeslice of the ui system on given core
		
		#if USE_UGUI
			if (nCore == CORE_FOR_UI_SYSTEM)
				runUISystem(nCore,false);
		#endif
		
		// on core0 increment loop counter and
		// notify when everything is setup
		
		if (nCore == 0)
		{
			main_loop_counter++;
			
			if (!bCore0StartupReported &&
				#if USE_UGUI
					m_bUIStarted &&
				#endif
				m_bAudioStarted)
			{
				bCore0StartupReported = 1;
				printf("ready ...\n");
				CScheduler::Get()->MsSleep(500);
					// to give the printf time before we change
					// the pin, just a minor aesthetic issue ...
			
				#if USE_GPIO_READY_PIN
					CGPIOPin toTeensy(USE_GPIO_READY_PIN,GPIOModeOutput);
					toTeensy.Write(1);
				#endif
			}
		}

	}	// while (1)
}	// CCoreTask::Run()


//---------------------------------------------
// AudioStream::update_all() IPI handler
//---------------------------------------------

#if CORE_FOR_AUDIO_SYSTEM != 0

	void CCoreTask::IPIHandler(unsigned nCore, unsigned nIPI)
	{
		if (nCore == CORE_FOR_AUDIO_SYSTEM &&
			nIPI == IPI_AUDIO_UPDATE)
		{
			AudioStream::do_update();
		}
	}
#endif



//----------------------------------------------
// Kernel Construction and Initialization
//----------------------------------------------

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
		#if USE_ALT_TOUCH_SCREEN
			// ,m_SPI(),
			,m_ili9846(&m_SPI)
			,m_xpt2046(&m_SPI)
			,m_GUI(0,&m_ili9846,&m_xpt2046)
		#else
			,m_GUI(&m_Screen)
		#endif
	#endif
	,m_CoreTask(this)
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
	
	#ifdef USE_MULTI_CORE
		if (bOK)
			bOK = m_CoreTask.Initialize();
	#endif
	
	#if USE_UGUI
		if (bOK)
		{
			#if USE_ALT_TOUCH_SCREEN
				m_SPI.Initialize();
				m_ili9846.Initialize ();
			#else
				m_TouchScreen.Initialize ();
			#endif
			bOK = m_GUI.Initialize ();
		}
	#endif
	
	return bOK;
}



TShutdownMode CKernel::Run(void)
	// calls the CoreTask to do all the work
{
	LOG("std_kernel " __DATE__ " " __TIME__ " available memory=%d",mem_get_size());

	m_CoreTask.Run(0);
	
	return ShutdownHalt;

}


