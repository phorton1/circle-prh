//
// std_kernel.h
//
// A standard kernel that implements arduino like setup() and loop()
// calls to minimize changes to teensy audio library test programs

//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _std_kernel_h
#define _std_kernel_h

#define USE_SCREEN  	 1
#define USE_USB          1
#define USE_UGUI         1		// requires USE_SCREEN and USE_USB
#define USE_MINI_SERIAL  0
#define USE_MAIN_SERIAL  1


#include <circle/memory.h>
#include <circle/actled.h>
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

#if USE_UGUI
	#include <audio/audio.h>	// for arduino.h NULL definition
	#include <circle/input/touchscreen.h>
	#include <ugui/uguicpp.h>
#endif


#ifdef ARM_ALLOW_MULTI_CORE
#define USE_MULTI_CORE
#endif




#ifdef USE_MULTI_CORE
	#include <circle/multicore.h>

	class CKernel;
	class CCoreTask : public CMultiCoreSupport
	{
		public:
			CCoreTask(CKernel *pKernel);
			~CCoreTask();
			void Run(unsigned nCore);
		private:
			CKernel *m_pKernel;
	};
#endif




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
	CActLED				m_ActLED;
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
	#if USE_UGUI
		CTouchScreenDevice	m_TouchScreen;
		CUGUI			m_GUI;
	#endif
	#ifdef USE_MULTI_CORE
		CCoreTask 	m_CoreTask;
	#endif
	
};


#endif	// _std_kernel_h
