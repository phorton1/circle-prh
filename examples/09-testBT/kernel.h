//
// kernel.h
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

#ifndef _kernel_h
#define _kernel_h

#define USE_SCREEN  0
	// Turn on the screen.
	// You can separately decide whether to use that as the logging/output devic.

// Note .. it should be possible to specify other pins for either serial device
// so that they can co-exist in an implementation instance. Right now they
// cannot, so you can only define one of the followin USE_SERIAL defines at a time.

#define USE_MINI_SERIAL  1
	// You can decide whether to have a mini uart device on pins 14 and 15
	// You can separately decide whether to use that as the logging/output devic.
	
#define USE_MAIN_SERIAL  0
	// You can decide whether to have a regular serial port device on pins 14 and 15
	// You can separately decide whether to use that as the logging/output devic.

#define USE_USB  0				
	// You can turn on or off the entire USB subsystem.
	// The kernel, at runtime, can separately decide whether to
	// use the (any given) USB bluetooth (transport) device, if
	// one is present, or it can decide to use the UART or miniUART
	// transport to the onboard bluetooth Controller.
	// Note that choice to construct a UART transport layer must
	// be on the device that is NOT being used for logging/output.


#include <circle/memory.h>
#include <utils/myActLED.h>
#include <utils/myUtils.h>
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
	#include <utils/miniuart.h>
#endif
#include <circle/logger.h>
#if USE_USB
	#include <circle/usb/dwhcidevice.h>
#endif
#include <circle/sched/scheduler.h>
#include <bt/bluetooth.h>
#include <circle/types.h>

#if	HCI_USE_FAT_DATA_FILE		// defined in hciLayer.h
	#include <SDCard/emmc.h>
	#include <fatfs/ff.h>	// use the addon library
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
	#if USE_USB
		CDWHCIDevice	m_DWHCI;
	#endif
	CScheduler			m_Scheduler;

#if HCI_USE_FAT_DATA_FILE
	CEMMCDevice			m_EMMC;
	FATFS				m_FileSystem;
#endif

	bluetoothAdapter	m_bt;
	
};

#endif
