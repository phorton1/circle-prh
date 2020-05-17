// std_kernel.cpp
//
// Based on Circle - A C++ bare metal environment for Raspberry Pi
// which is Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
//
// This code is Copyright (C) 2019, Patrick Horton, no rights reserved.


#include "std_kernel.h"
#include <circle/util.h>
#include <circle/types.h>
#include <circle/alloc.h>
#include <circle/gpiopin.h>


#if USE_AUDIO_SYSTEM
	#include <audio/AudioStream.h>
#endif


#if USE_FILE_SYSTEM
	#define SHOW_ROOT_DIRECTORY  1
	#define DRIVE		"SD:"
#endif


#define USE_GPIO_READY_PIN    25		// 25

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
bool bCore0StartupReported = 0;


//---------------------------------------------
// CoreTask
//---------------------------------------------

CCoreTask *CCoreTask::s_pCoreTask = 0;

u32 ctor_sp = 0;

CCoreTask::CCoreTask(CKernel *pKernel)	:
	#ifdef WITH_MULTI_CORE
		CMultiCoreSupport(&pKernel->m_Memory),
	#endif
	m_pKernel(pKernel)
	#if USE_AUDIO_SYSTEM
		,m_bAudioStarted(0)
	#endif
	#if USE_UI_SYSTEM
		,m_bUIStarted(0)
	#endif
{
	s_pCoreTask = this;


	#if 0
		u32 sp;
		asm volatile
		(
			"mov %0,sp\n"
			: "=r" (sp)
		);
		ctor_sp = sp;;
	#endif	
	
}


CCoreTask::~CCoreTask()
{
	m_pKernel = 0;
}

#if USE_FILE_SYSTEM
	FATFS *CCoreTask::GetFileSystem()
	{
		return &m_pKernel->m_FileSystem;
	}
#endif


#if USE_AUDIO_SYSTEM
	// audio system currently includes pseudo arduino api
	void CCoreTask::runAudioSystem(unsigned nCore, bool init)
	{
		if (init)
		{
			LOG("AudioSystem starting on Core(%d) mem=%dM",nCore,mem_get_size()/1000000);
			setup();
			LOG("after AudioSystem started mem=%dM",mem_get_size()/1000000);
			m_bAudioStarted = 1;
		}
		else
		{
			loop();
		}
	}
#endif



#if USE_UI_SYSTEM

	#define UI_FRAME_RATE   60
		// undefine this to not throttle the UI

	void CCoreTask::runUISystem(unsigned nCore, bool init)
	{
		if (init)
		{
			#if USE_AUDIO_SYSTEM
				while (!m_bAudioStarted);
			#endif
			
			LOG("UI starting on Core(%d) mem=%dM",nCore,mem_get_size()/1000000);
			delay(1000);

			CMouseDevice *pMouse = (CMouseDevice *) CDeviceNameService::Get ()->GetDevice ("mouse1", FALSE);
			
			#ifdef WITH_480x320_ILI9486_XPT2046_TOUCHSCREEN
				CScreenDeviceBase *pUseScreen = &m_pKernel->m_ili9486;
				CTouchScreenBase  *pTouch = &m_pKernel->m_xpt2046;
			#else
				CScreenDeviceBase *pUseScreen = &m_pKernel->m_Screen;
				CTouchScreenBase  *pTouch = (CTouchScreenBase *) CDeviceNameService::Get ()->GetDevice ("touch1", FALSE);
			#endif
		
			m_pKernel->m_app.Initialize(pUseScreen,pTouch,pMouse);
			LOG("after UI initialization mem=%dM",mem_get_size()/1000000);
			
			m_bUIStarted = 1;
		}
		else
			#if USE_AUDIO_SYSTEM
				if (m_bAudioStarted)
			#endif
		{
			#ifdef UI_FRAME_RATE
			static u32 ui_timer = 0;
			u32 now = m_pKernel->m_Timer.GetClockTicks();
			if (now > ui_timer + (1000000/UI_FRAME_RATE))
			{
				ui_timer = now;
			#endif

				m_pKernel->m_app.timeSlice();

			#if UI_FRAME_RATE
			}
			#endif
		}
	}
#endif



void CCoreTask::Run(unsigned nCore)
{
	LOG("Core(%d) starting ... mem=%dM",nCore,mem_get_size()/1000000);
	dprobe(0,"Core(%d) starting",nCore);
	
	#if 0
		u32 sp;
		asm volatile
		(
			"mov %0,sp\n"
			: "=r" (sp)
		);
		printf("  core(%d) SP(%08x)\n",nCore,sp);
	#endif	
		
	// initialize the audio system on the given core
	
	#if USE_AUDIO_SYSTEM
		if (nCore == CORE_FOR_AUDIO_SYSTEM)
		{
			runAudioSystem(nCore,true);
			dprobe(1,"after first runAudioSystem",0);
		}
	#endif
	
	// initialilze the ui on the given core
	
	#if USE_UI_SYSTEM
		if (nCore == CORE_FOR_UI_SYSTEM)
		{
			runUISystem(nCore,true);
			dprobe(1,"after first runUISystem",0);
		}
	#endif
	
	delay(500);
	dprobe(0,"CCoreTask::Run(%d) before loop",nCore);

	while (1)
	{
		// do a timeslice of the audio system on given core
		// if it's not core 0, this will just call Arduion loop()
		// method very rapidly, as audio will be handled by
		// inter-processor interrupts

		#if USE_AUDIO_SYSTEM
			if (nCore == CORE_FOR_AUDIO_SYSTEM)
			{
				runAudioSystem(nCore,false);
				// dprobe(0,"after runAudioSystem",0);
			}
		#endif
		
		// do a timeslice of the ui system on given core
		
		#if USE_UI_SYSTEM
			if (nCore == CORE_FOR_UI_SYSTEM)
			{
				runUISystem(nCore,false);
				// dprobe(0,"after runUISystem",0);
			}
		#endif
		
		// on core0 increment loop counter and
		// notify when everything is setup
		
		if (nCore == 0)
		{
			main_loop_counter++;
			
			if (!bCore0StartupReported
				#if USE_UI_SYSTEM
					&& m_bUIStarted 
				#endif
				#if USE_AUDIO_SYSTEM
					&& m_bAudioStarted
				#endif
				)
			{
				CScheduler::Get()->MsSleep(200);
				printf("ready ...\n");
				CScheduler::Get()->MsSleep(200);
					// to give the printf time before we change
					// the pin, just a minor aesthetic issue ...
			
				#if USE_GPIO_READY_PIN
					CGPIOPin toTeensy(USE_GPIO_READY_PIN,GPIOModeOutput);
					toTeensy.Write(1);
				#endif
				
				// dprobe(2,"CoreTask(0) rpi ready",0);

				bCore0StartupReported = 1;
			}
		}

		#ifdef DPROBE
			if (nCore == 3)
			{
				static bool bProbeReported = false;
				if (bCore0StartupReported && !bProbeReported)
				{
					bProbeReported = true;
					dprobe(2,"Core(3) ready",0);
				}
				else
				{
					dprobe(0,"Core(3) at end of CoreTask(0)::Run() loop",0);
					delay(1000);
				}
			}
		#endif
		
	}	// while (1)
}	// CCoreTask::Run()


//---------------------------------------------
// AudioStream::update_all() IPI handler
//---------------------------------------------

#if USE_AUDIO_SYSTEM
	#if CORE_FOR_AUDIO_SYSTEM != 0
	
		void CCoreTask::IPIHandler(unsigned nCore, unsigned nIPI)
		{
			if (nCore == CORE_FOR_AUDIO_SYSTEM &&
				nIPI == IPI_AUDIO_UPDATE)
			{
				AudioSystem::doUpdate();
			}
		}
	#endif
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
	
	// prh - 2020-05-03   Logging seems to work WAY better without the interrupt system !
	// THIS HAS NOT BEEN TESTED WITH THE BOOTLOADER, WHICH *MIGHT* NEED THE INTERRUPTS
	// but conveniently has it's own Kernel.
	
	// EXCEPT IT (not using interrupts) CAUSED ALL KINDS OF NOISE IN THE AUDIO SYSTEM
	// GRR ... and I changed back to multi-core too ..
	
	
	#if USE_MAIN_SERIAL
		m_Serial(&m_Interrupt, FALSE),	// TRUE),	// (0,FALSE,
	#endif
	m_Logger(LogDebug,&m_Timer)	// m_Options.GetLogLevel(), &m_Timer)
	#if USE_USB
		,m_DWHCI(&m_Interrupt, &m_Timer)
	#endif
	#if USE_UI_SYSTEM
		#ifdef WITH_480x320_ILI9486_XPT2046_TOUCHSCREEN
			,m_ili9486(&m_SPI)
			,m_xpt2046(&m_SPI)
		#endif
	#endif
	,m_CoreTask(this)
	#if USE_FILE_SYSTEM
		,m_EMMC(&m_Interrupt, &m_Timer, &m_ActLED)
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

	dprobe(0,"",0);
	
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

	dprobe(0,"after logger started",0);
	
	#if USE_USB
		if (bOK)
			bOK = m_DWHCI.Initialize ();
	#endif

	#if 0
		#ifdef WITH_MULTI_CORE
			if (bOK)
				bOK = m_CoreTask.Initialize();
		#endif
	#endif
	
	#if USE_UI_SYSTEM
		if (bOK)
		{
			#ifdef WITH_480x320_ILI9486_XPT2046_TOUCHSCREEN
				m_SPI.Initialize();
				m_ili9486.Initialize ();
			#else
				m_TouchScreen.Initialize ();
			#endif
		}
	#endif
	
	#if USE_FILE_SYSTEM
		if (bOK)
		{
			bOK = m_EMMC.Initialize();
			initFileSystem();
		}
	#endif

	
	#if 0
		#ifdef WITH_MULTI_CORE
			if (bOK)
				bOK = m_CoreTask.Initialize();
		#endif
	#endif
	
	return bOK;
}




#include <circle/memorymap.h>

TShutdownMode CKernel::Run(void)
	// calls the CoreTask to do all the work
{
	LOG("std_kernel " __DATE__ " " __TIME__ " available memory=%d",mem_get_size());
	
	#if 0
		u32 sp;
		asm volatile
		(
			"mov %0,sp\n"
			: "=r" (sp)
		);
		printf("MEM_KERNEL_STACK(%08x) SP(%08x) ctor(%08d)\n",MEM_KERNEL_STACK,sp,ctor_sp);
	#endif

	#if USE_MIDI_SYSTEM
		#if USE_MAIN_SERIAL
			m_MidiSystem.Initialize(&m_Serial);
		#else
			m_MidiSystem.Initialize();
		#endif
	#endif
	
	
	#if 1
		delay(500);
		m_CoreTask.Initialize();
		delay(500);
	#endif
	
	m_CoreTask.Run(0);
	
	return ShutdownHalt;

}



#if USE_FILE_SYSTEM
	void CKernel::initFileSystem()
	{
		if (f_mount(&m_FileSystem, DRIVE, 1) != FR_OK)
		{
			LOG_ERROR("Cannot mount drive: %s", DRIVE);
			return;
		}
	
		#if SHOW_ROOT_DIRECTORY
			LOG("Contents of SD card",0);
			DIR Directory;
			FILINFO FileInfo;
			FRESULT Result = f_findfirst (&Directory, &FileInfo, DRIVE "/", "*");
			for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
			{
				if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
				{
					LOG("%-22s %ld", FileInfo.fname, FileInfo.fsize);
				}
				Result = f_findnext (&Directory, &FileInfo);
			}
		#endif
	}
#endif

