//
// std_kernel.h
//
// A standard kernel that:
//
// - Implements the basic UI timeSlice() loop.
// - also implements arduino like setup() and loop()
//   calls to minimize changes to teensy audio library test programs.
//
// Based on Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (c) 2015-2016  R. Stange <rsta2@o2online.de>
// Copyright (c) 2019 Patrick Horton- no rights reserved, 
// please see license details in LICENSE.TXT

#ifndef _std_kernel_h
#define _std_kernel_h

// Notes on defines:
//
// Defines that start with USE_ have 0/1 values, one of which MUST be set
// Defines that start with WITH_ may be defined, or not
//
// At this time the defines here must be coordinated with the compilation
// of the various example programs.  For instance, USE_AUDIO_SYSTEM must
// be set to 1 in order to compile and link any of the examples that use
// audio.
//
// If USE_UI_SYSTEM is defined, then someone must implement a method
// wsApplication::Create() which defines the windows in the UI.
// An empty method can be provided by includingstd_empty_ui.h.
//
// I don't like the way this works.
// I am considering a compilation strategy whereby all variants
// are compiled to objects of different names, and then the
// programs link against the pieces they want.


#define USE_UI_SYSTEM 		1			
#define USE_AUDIO_SYSTEM 	1
#define USE_MIDI_SYSTEM     1			// requires USE_USB

#define USE_SCREEN  	 	1			// may run with only specific i2c/spi touch screen devices
#define USE_USB          	1			// may run with, or without, a mouse
#define USE_MINI_SERIAL  	0			// can output log to either serial port
#define USE_MAIN_SERIAL  	1
#define USE_FILE_SYSTEM     0			// include (and initalize) the addon fatfs


// At most, one of the following may be defined/
// If USE_UI_SYSTEM then USE_SCREEN or one of these MUST be defined.
//
// The following defines override the binding of the user
// interface to physical devices.  By defaut, it expects
// a bcm hdmi circle CScreen device, and will automatically
// use the Circle default official rpi touchscreen if one is
// present.  If USE_USB is defined it will additionally bind
// the UI to a mouse device.
//
// If one of the below is defined, the CScreen, nor the default
// touchscreen, nor the mouse will be bound to the UI.

// #define WITH_480x320_ILI9486_XPT2046_TOUCHSCREEN 
	// This define corresponds to the standard cheap resistive
	// rpi touch screen that I implemented.

#include <circle/memory.h>
#include <utils/myActLED.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#if USE_SCREEN
	#include <circle/screen.h>
#endif
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#if USE_MAIN_SERIAL
	#include <circle/serial.h>
#endif
#if USE_MINI_SERIAL
	#include <circle/miniuart.h>
#endif
#include <circle/logger.h>
#include <circle/sched/scheduler.h>

#if USE_USB
	#include <circle/usb/dwhcidevice.h>
#endif

#if USE_UI_SYSTEM
	#include <ws/wsApp.h>
	#ifdef WITH_480x320_ILI9486_XPT2046_TOUCHSCREEN
		#include <devices/ili9486.h>
		#include <devices/xpt2046.h>
	#else
		#include <circle/input/touchscreen.h>
	#endif
#endif

#if USE_FILE_SYSTEM
	#include <SDCard/emmc.h>
	#include <fatfs/ff.h>
#endif

#if USE_MIDI_SYSTEM
	#include "midiEvent.h"
#endif


#ifdef ARM_ALLOW_MULTI_CORE
#define WITH_MULTI_CORE
#endif


#ifdef WITH_MULTI_CORE
	#include <circle/multicore.h>

	#define CORE_FOR_AUDIO_SYSTEM    1
		// The Audio System hardware interrupts (bcm_pcm) always
		// take place on Core 0, as do the USB interrupts.
		// If the audio system runs on Core 0, then do_update()
		// is called directly from the audio IRQ (update responsibility)
		//
		// If it is running on a different core, an inter-processor
		// interrupt is triggered from the core0 code (IRQ) to the
		// core which will just call do_update on the IPI.
		//
		// As currently implemented, the psudo-Arudino setup() and
		// loop() calls are made from the core running the Audio
		// system.
		
	#define CORE_FOR_UI_SYSTEM       2
		// The UI System is updated in the Run() loop on the
		// given core.

#else	// Single Core defines
	#define CORE_FOR_AUDIO_SYSTEM    0
	#define CORE_FOR_UI_SYSTEM       0
#endif


#if CORE_FOR_AUDIO_SYSTEM != 0
	#define IPI_AUDIO_UPDATE  11		// first user IPI + 1 (arbitrary upto 30)
#endif



class CKernel;
class CCoreTask 
	#ifdef WITH_MULTI_CORE
		: public CMultiCoreSupport
	#endif
{
	public:
		
		CCoreTask(CKernel *pKernel);
		~CCoreTask();
		void Run(unsigned nCore);
		static CCoreTask *Get() {return s_pCoreTask;}
		
		#if USE_AUDIO_SYSTEM
			#if CORE_FOR_AUDIO_SYSTEM != 0
				void IPIHandler(unsigned nCore, unsigned nIPI);
			#endif
		#endif
		
		#if USE_FILE_SYSTEM
			FATFS *GetFileSystem();
		#endif
		
	private:

		CKernel *m_pKernel;
		static CCoreTask *s_pCoreTask;
		
	#if USE_AUDIO_SYSTEM
		void runAudioSystem(unsigned nCore, bool init);
		volatile bool m_bAudioStarted;
	#endif
	
	#if USE_UI_SYSTEM
		void runUISystem(unsigned nCore, bool init);
		volatile bool m_bUIStarted;
	#endif
};




enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

class CKernel 
{
public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);

	TShutdownMode Run (void);

private:
	
	friend class CCoreTask;

	CMemorySystem		m_Memory;
	myActLED			m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	
	#if USE_SCREEN
		CScreenDevice	m_Screen;
	#endif
	
	#if USE_MINI_SERIAL
		CMiniUartDevice m_MiniUart;
	#endif
	
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer				m_Timer;
	
	#if USE_MAIN_SERIAL
		CSerialDevice	m_Serial;
	#endif
	
	CLogger				m_Logger;
	CScheduler			m_Scheduler;
	
	#if USE_USB
		CDWHCIDevice	m_DWHCI;
	#endif
	
	#if USE_UI_SYSTEM
		wsApplication 	m_app;
		#ifdef WITH_480x320_ILI9486_XPT2046_TOUCHSCREEN
			CSPIMaster	m_SPI;
			ILI9846 	m_ili9486;
			XPT2046 	m_xpt2046;
		#else	
			CTouchScreenDevice	m_TouchScreen;
		#endif
	#endif

	CCoreTask 	m_CoreTask;
	
	#if USE_FILE_SYSTEM
		CEMMCDevice			m_EMMC;
		FATFS				m_FileSystem;
		void initFileSystem();
	#endif
	
	
	#if USE_MIDI_SYSTEM
		midiSystem m_MidiSystem;
	#endif
	
};


#endif	// _std_kernel_h
