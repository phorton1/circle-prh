//
// bootloader.h
// from a typical kernel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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

#define  USE_CIRCLE_FAT   0
	// as opposed to addon/fatfs

#define WITH_FILE         1
#define WITH_SCREEN 	  0
#define WITH_HTTP  		  0
#define WITH_TFTP  		  0
#define WITH_SOFT_SERIAL  0

#include <circle/types.h>
#include <circle/stdarg.h>
#include <circle/memory.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>

#if WITH_SCREEN
	#include <circle/screen.h>
#endif

#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/serial.h>

#if WITH_FILE
	#include <SDCard/emmc.h>
	#if USE_CIRCLE_FAT
		#include <circle/fs/fat/fatfs.h>
	#else
		#include <fatfs/ff.h>	// use the addon library instead
	#endif
#endif

#if WITH_TFTP || WITH_HTTP
	#include <circle/sched/scheduler.h>
	#include <circle/usb/dwhcidevice.h>
	#include <circle/net/netsubsystem.h>
#endif

#if WITH_SOFT_SERIAL
	#include <circle/gpiomanager.h>
		// something in gpioManager is also stopping chain booting
	#include "softserial.h"
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
	
	CDevice *m_pUseSerial;
	
	void waitForUpload();
	void readBinarySerial();
	bool read32Serial(u32 *retval);

#if WITH_FILE
	void initFileSystem();
	void closeFileSystem();
	void readKernelFromSD();
	void writeKernelToSD();
#endif

	void debugDumpEnvironment();
	
	u8 *m_pKernelBuffer;
	u32 m_kernel_size;
	u32 m_kernel_mod_time;
	u32 m_kernel_create_time;
	CString m_kernel_filename;
	
	// "do not change this order"
	
	CMemorySystem		m_Memory;
	CActLED				m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	CCPUThrottle		m_CPUThrottle;
#if WITH_SCREEN
	CScreenDevice		m_Screen;
#endif
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer				m_Timer;
	CSerialDevice 		m_Serial;
	CLogger				m_Logger;
#if WITH_SOFT_SERIAL
	CGPIOManager		m_GPIOManager;
	CSoftSerialDevice	m_SoftSerial;
#endif
#if WITH_TFTP || WITH_HTTP
	CScheduler			m_Scheduler;
	CDWHCIDevice		m_DWHCI;
	CNetSubSystem		m_Net;
#endif
#if WITH_FILE
	CEMMCDevice			m_EMMC;
	#if USE_CIRCLE_FAT
		CFATFileSystem	m_FileSystem;
	#else
		FATFS			m_FileSystem;
	#endif
#endif

};

#endif	// _kernel_h
