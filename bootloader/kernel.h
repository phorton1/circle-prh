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


#define WITH_SCREEN 	  0
	// if so, binary protocol debugging will be sent to the screen
	// rather than the "log" device (serial port).

#include <circle/types.h>
#include <circle/stdarg.h>
#include <circle/memory.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
// #include <utils/myUtils.h>
#include <utils/myActLED.h>

#if WITH_SCREEN
	#include <circle/screen.h>
#endif

#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/serial.h>
#include <SDCard/emmc.h>
#include <fatfs/ff.h>




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
	
	CDevice *m_pSerialDevice;
	CDevice *m_pSerialDebug;
	
	void waitForUpload();
	void readBinarySerial();
	bool read32Serial(u32 *retval);

	void initFileSystem();
	void closeFileSystem();
	void readKernelFromSD();
	void writeKernelToSD();

	void debugDumpEnvironment();
	
	u8 *m_pKernelBuffer;
	u32 m_kernel_size;
	u32 m_kernel_mod_time;
	u32 m_kernel_create_time;
	CString m_kernel_filename;
	
	// "do not change this order"
	
	CMemorySystem		m_Memory;
	myActLED			m_ActLED;
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
	CEMMCDevice			m_EMMC;
	FATFS				m_FileSystem;

	void dbg_serial(const char *pMessage, ...);

};

#endif	// _kernel_h
