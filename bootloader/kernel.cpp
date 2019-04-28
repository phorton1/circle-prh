//
// kernel.cpp - prh
//
//     this file is compiled and linked to recovery.img
//
//     the standard stock raspberry pi bootcode.bin and
//     start.elf scheme attempts to load recovery.img before
//     kernel.img
//
//     so by calling our program recovery.img, we can allow
//     folks to create the expected kernel.img's that are common
//     to most bare metal makefile projects, which we can then
//     chain boot to.
//
// This bootloader is based on ...
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
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

#define WRITE_KERNEL 		 0
#define SHOW_ROOT_DIRECTORY  0

#include "kernel.h"
#include <assert.h>
#include <circle/util.h>
#include <circle/alloc.h>
#include <circle/chainboot.h>
#include <circle/sysconfig.h>

#if WITH_TFTP
	#include "tftpbootserver.h"
	CTFTPBootServer *pTFTP = 0;
#endif
#if WITH_HTTP
	#include "httpbootserver.h"
	#define HTTP_BOOT_PORT		8080
	CHTTPBootServer *pHTTP = 0;
#endif



#define logBoot 	"boot"

#if USE_CIRCLE_FAT
	#define PARTITION	"emmc1-1"
#else
	#define DRIVE		"SD:"
#endif


#if RASPPI == 3
	#define KERNEL_FILENAME	 "kernel8-32.img"		// wont work - long filenames not supported in circle
#elif RASPPI == 2
	#define KERNEL_FILENAME  "kernel7.img"
#else
	#define KERNEL_FILENAME  "kernel.img"
#endif


#if WITH_TFTP || WITH_HTTP
	// #define USE_DHCP
	#ifndef USE_DHCP
	static const u8 IPAddress[]      = {192, 168, 0, 250};
	static const u8 NetMask[]        = {255, 255, 255, 0};
	static const u8 DefaultGateway[] = {192, 168, 0, 1};
	static const u8 DNSServer[]      = {192, 168, 0, 1};
	#endif
#endif



//----------------------------------------------
// basics
//----------------------------------------------

CKernel::~CKernel(void)
{}


CKernel::CKernel(void) :

#if WITH_SCREEN
	m_Screen(m_Options.GetWidth(), m_Options.GetHeight()),
#endif
	m_Timer(&m_Interrupt),
	m_Serial(&m_Interrupt, TRUE),
	m_Logger(m_Options.GetLogLevel())	// , &m_Timer)
#if WITH_HTTP || WITH_TFTP
	,m_DWHCI(&m_Interrupt, &m_Timer)
	#ifndef USE_DHCP
		,m_Net(IPAddress, NetMask, DefaultGateway, DNSServer)
	#endif
#endif
#if WITH_SOFT_SERIAL
	,m_GPIOManager(&m_Interrupt)
	,m_SoftSerial(18, 17, &m_GPIOManager)
#endif
#if WITH_FILE
	,m_EMMC(&m_Interrupt, &m_Timer, &m_ActLED)
#endif
{
	m_pUseSerial = 0;

	m_kernel_size = 0;
	m_pKernelBuffer = 0;
	m_kernel_mod_time = 0;
	m_kernel_create_time = 0;
	m_ActLED.Toggle();	// Blink(6,50,100);
}


//----------------------------------------------
// initialize()
//----------------------------------------------

boolean CKernel::Initialize(void)
{
	boolean bOK = TRUE;
	// color == light cyan
	m_Logger.setDefaultAnsiColorCode(96);

	// note that I changed logger.cpp to NOT write to pTarget
	// if the logger has not been initialized yet.  Otherwise,
	// without a screen, it would die in other things that
	// called CLogger()->Write() ...
	
	#if WITH_SCREEN
		if (bOK)
		{
			bOK = m_Screen.Initialize();
			m_ActLED.Toggle();
		}
	#endif

	// initialize interrupts, timers, and the serial port(s)
	
	if (bOK)
	{
		bOK = m_Interrupt.Initialize();
		m_ActLED.Toggle();
	}
	if (bOK)
	{
		bOK = m_Timer.Initialize();
		m_ActLED.Toggle();
	}
	
	if (bOK)
	{
		bOK = m_Serial.Initialize(115200);	// 115200);	// 921600);
		m_pUseSerial = &m_Serial;
		m_ActLED.Toggle();
	}
	
	// determine where to log and print based on cmdline.txt
	// though I default to serial instead of screen
	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Serial;
		}
		bOK = m_Logger.Initialize (pTarget);
		m_ActLED.Toggle();
	}		
	
	// and finally, initialize other tricky subsystems

	#if WITH_HTTP || WITH_TFTP
		if (bOK)
		{
			bOK = m_DWHCI.Initialize();
			m_ActLED.Toggle();
		}
		
		if (bOK)
		{
			bOK = m_Net.Initialize();
			m_ActLED.Toggle();
		}
	#endif

	#if WITH_FILE
		if (bOK)
		{
			bOK = m_EMMC.Initialize();
			m_ActLED.Toggle();
		}
	#endif
	
	#if WITH_SOFT_SERIAL
		// SoftSerial::Initialize() should take a baud_rate
		// and the object should be moved into circle/include and lib

		if (bOK)
		{
			printf("starting soft serial ..\n");
			bOK = m_SoftSerial.Initialize();
			printf("soft serial started\n");
			
			m_Timer.MsDelay(1000);
			m_ActLED.Toggle();
			m_SoftSerial.Write("prh238\n",7);
			m_Timer.MsDelay(1000);
			m_ActLED.Toggle();

			m_pUseSerial = &m_SoftSerial;
			bOK = m_Logger.Initialize(&m_SoftSerial);
			m_ActLED.Toggle();

		}
	#endif
		
	return bOK;
}


//----------------------------------------------
// file routines
//----------------------------------------------

#if WITH_FILE
	
	void CKernel::closeFileSystem()
		// Unmount file system
	{
		#if USE_CIRCLE_FAT
			m_FileSystem.Synchronize();
			m_FileSystem.UnMount();
		#else
			if (f_mount (0, DRIVE, 0) != FR_OK)
			{
				m_Logger.Write (logBoot, LogPanic, "Cannot unmount drive: %s", DRIVE);
			}
		#endif
	}
	
	
	void CKernel::initFileSystem()
	{
		#if SHOW_ROOT_DIRECTORY
			m_Logger.Write(logBoot, LogNotice, "Contents of SD card");
		#endif
				
		#if USE_CIRCLE_FAT
		
			CDevice *pPartition = m_DeviceNameService.GetDevice(PARTITION, TRUE);
			if (pPartition == 0)
			{
				m_Logger.Write(logBoot, LogPanic, "Partition not found: %s", PARTITION);
			}
			else if (!m_FileSystem.Mount(pPartition))
			{
				m_Logger.Write(logBoot, LogPanic, "Cannot mount partition: %s", PARTITION);
			}
			else
			{
				m_ActLED.Toggle();
				TDirentry Direntry;
				TFindCurrentEntry CurrentEntry;
				unsigned nEntry = m_FileSystem.RootFindFirst(&Direntry, &CurrentEntry);
				for (unsigned i = 0; nEntry != 0; i++)
				{
					if (!(Direntry.nAttributes & FS_ATTRIB_SYSTEM))
					{
						#if SHOW_ROOT_DIRECTORY
							m_Logger.Write(logBoot, LogNotice, "%-14s %ld", Direntry.chTitle, Direntry.nSize);
						#endif
						
						if (!strcmp(Direntry.chTitle,KERNEL_FILENAME))
						{
							m_Logger.Write(logBoot, LogNotice, "FOUND KERNEL %s %ld", Direntry.chTitle, Direntry.nSize);
							m_kernel_size = Direntry.nSize;
						}
					}
					nEntry = m_FileSystem.RootFindNext(&Direntry, &CurrentEntry);
				}
		
				m_ActLED.Toggle();
				if (!m_kernel_size)
				{
					m_Logger.Write(logBoot, LogPanic, "WARNING - could not find KERNEL %s %ld", KERNEL_FILENAME);
				}
				else if (m_kernel_size > KERNEL_MAX_SIZE)
				{
					m_Logger.Write(logBoot, LogPanic, "ERROR - KERNEL %ld is too big %ld", m_kernel_size, KERNEL_MAX_SIZE);
					m_kernel_size = 0;
				}
			}
			
		#else	// use addon FATFS
		
			if (f_mount(&m_FileSystem, DRIVE, 1) != FR_OK)
			{
				m_Logger.Write(logBoot, LogPanic, "Cannot mount drive: %s", DRIVE);
				return;
			}
		
			DIR Directory;
			FILINFO FileInfo;
			FRESULT Result = f_findfirst (&Directory, &FileInfo, DRIVE "/", "*");
			for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
			{
				if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
				{
					#if SHOW_ROOT_DIRECTORY
						m_Logger.Write(logBoot, LogNotice, "%-14s %ld", FileInfo.fname, FileInfo.fsize);
					#endif
					
					if (!strcmp(FileInfo.fname,KERNEL_FILENAME))
					{
						m_Logger.Write(logBoot, LogNotice, "FOUND KERNEL %s %ld", FileInfo.fname, FileInfo.fsize);
						m_kernel_size = FileInfo.fsize;
					}
				}
				Result = f_findnext (&Directory, &FileInfo);
			}
			
		#endif
	}
	
	
	void CKernel::readKernelFromSD()
	{
		m_ActLED.Toggle();
		m_Logger.Write(logBoot, LogDebug, "Loading %s[%d]", KERNEL_FILENAME,m_kernel_size);
		
		#if USE_CIRCLE_FAT
	
			unsigned hFile = m_FileSystem.FileOpen(KERNEL_FILENAME);
			if (hFile)
			{
				m_pKernelBuffer = new u8[m_kernel_size];
				if (m_pKernelBuffer)
				{
					unsigned int got = m_FileSystem.FileRead(hFile,m_pKernelBuffer,m_kernel_size);
					if (got == m_kernel_size)
					{
						m_FileSystem.FileClose(hFile);
						m_Logger.Write(logBoot, LogNotice,"enabling chain boot for %s",KERNEL_FILENAME);
						EnableChainBoot(m_pKernelBuffer, m_kernel_size);
					}
					else
					{
						m_Logger.Write(logBoot, LogPanic, "ERROR - got %d expected %d",got,m_kernel_size);
						free(m_pKernelBuffer);
						m_pKernelBuffer	= 0;
					}
				}
				else
				{
					m_Logger.Write(logBoot, LogPanic, "ERROR - could not allocate %d bytes",m_kernel_size);
				}
			}
			else
			{
				m_Logger.Write(logBoot, LogPanic, "ERROR - could not open %s",KERNEL_FILENAME);
			}
			
		#else	// READ THE FILE USING THE ADDON FAT FS
		
			FIL file;
			if (f_open(&file, DRIVE KERNEL_FILENAME, FA_READ | FA_OPEN_EXISTING) == FR_OK)
			{
				m_pKernelBuffer = new u8[m_kernel_size];
				if (m_pKernelBuffer)
				{
					u32 got;
					if (f_read(&file, m_pKernelBuffer, m_kernel_size, &got) == FR_OK)
					{
						if (got == m_kernel_size)
						{
							f_close(&file);
							m_Logger.Write(logBoot, LogNotice,"enabling FATFS chain boot for %s",KERNEL_FILENAME);
							EnableChainBoot(m_pKernelBuffer, m_kernel_size);
							
						}
						else
						{
							m_Logger.Write(logBoot, LogPanic, "ERROR - got %d expected %d",got,m_kernel_size);
							free(m_pKernelBuffer);
							m_pKernelBuffer	= 0;
						}
					}
					else
					{
						m_Logger.Write(logBoot, LogPanic, "ERROR - could not read ",m_kernel_size);
					}
				}
				else
				{
					m_Logger.Write(logBoot, LogPanic, "ERROR - could not allocate %d bytes",m_kernel_size);
				}
			}
			else
			{
				m_Logger.Write(logBoot, LogPanic, "ERROR - could not open %s",KERNEL_FILENAME);
			}
		
		#endif
	}
	
	
	#if WRITE_KERNEL
		void CKernel::writeKernelToSD()
			// if we got a kernel above, save it
			// as determined by IsChainBootEnabled() at this point
		{
			m_Logger.Write(logBoot, LogDebug, "saving kernel %s", (const char *) m_kernel_filename);
			
			#if USE_CIRCLE_FAT
			
				#define NEW_KERNEL_FILENAME "new_kern.img"
				unsigned hFile = m_FileSystem.FileCreate(NEW_KERNEL_FILENAME);
				if (hFile)
				{
					m_Logger.Write(logBoot, LogDebug, "opened %s", NEW_KERNEL_FILENAME);
					if (m_FileSystem.FileWrite(hFile,m_pKernelBuffer,m_kernel_size) == m_kernel_size)
					{
						m_Logger.Write(logBoot, LogDebug, "wrote %ld bytes ...", m_kernel_size);
						if (m_FileSystem.FileClose(hFile))
						{
							m_Logger.Write(logBoot, LogDebug, "NEW KERNEL %s WRITTEN!!", NEW_KERNEL_FILENAME);
						}
						else
						{
							m_Logger.Write(logBoot, LogPanic, "Cannot close file");
						}
					}
					else
					{
						m_Logger.Write(logBoot, LogError, "Write error");
					}
				}
				else	// hFile == 0
				{
					m_Logger.Write(logBoot, LogPanic, "Cannot create %s!!",NEW_KERNEL_FILENAME);
				}
	
			#else	// use addon FATFS library
			
				FIL file;
				FILINFO info;
				// prh removed DRIVE from all these names
				
				if (f_open(&file, m_kernel_filename, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
				{
					u32 wrote = 0;
					m_Logger.Write(logBoot, LogDebug, "Writing %d bytes to %s",m_kernel_size,(const char *) m_kernel_filename);
					if (f_write(&file,m_pKernelBuffer, m_kernel_size, &wrote) == FR_OK)
					{
						m_Logger.Write(logBoot, LogDebug, "file written ...");
						if (wrote == m_kernel_size)
						{
							m_Logger.Write(logBoot, LogDebug, "length verified ...");
							if (f_close(&file) == FR_OK)
							{
								m_Logger.Write(logBoot, LogDebug, "file closed ...");
								if (f_stat(m_kernel_filename, &info) == FR_OK)
								{
									//   The FAT date and time is a 32-bit value containing two 16-bit values:
									//     * The date (lower 16-bit).
									//       * bits 0 - 4:  day of month, where 1 represents the first day
									//       * bits 5 - 8:  month of year, where 1 represent January
									//       * bits 9 - 15: year since 1980
									//     * The time of day (upper 16-bit).
									//       * bits 0 - 4: seconds (in 2 second intervals)
									//       * bits 5 - 10: minutes
									//       * bits 11 - 15: hours
									
									m_Logger.Write(logBoot, LogDebug, "got f_stat fdate(%d) ftime(%d)",
										info.fdate,
										info.ftime);
									info.fdate = (m_kernel_mod_time >> 16) & 0xffff;
									info.ftime = m_kernel_mod_time & 0xffff;
									
									if (f_utime (m_kernel_filename, &info) == FR_OK)
									{
										m_Logger.Write(logBoot, LogDebug, "back from f_utime()");
									}
									else
									{
										m_Logger.Write(logBoot, LogPanic, "ERROR - f_utime(%s) failed",(const char *) m_kernel_filename);
									}
								}
								else
								{
									m_Logger.Write(logBoot, LogPanic, "ERROR - could not fstat(%s)",(const char *) m_kernel_filename);
								}
							}
							else
							{
								m_Logger.Write(logBoot, LogPanic, "ERROR - could not close %s",(const char *) m_kernel_filename);
							}
						}
						else
						{
							m_Logger.Write(logBoot, LogPanic, "ERROR - wrote %d expected %d",wrote,m_kernel_size);
						}
					}
					else
					{
						m_Logger.Write(logBoot, LogError, "Write error");
					}
				}
				else
				{
					m_Logger.Write(logBoot, LogPanic, "Cannot create %s!!",(const char *) m_kernel_filename);
				}
	
			
			#endif	// !USE_CIRCLE_FAT (use addon FATFS)
		}	// writeKernelToSD()
	#endif	// WRITE_KERNEL

#endif // WITH_FILE


//----------------------------------------
// memory routines
//----------------------------------------

void CKernel::waitForUpload()
{
	#define REBOOT_TIME   15
	#define TIME_SCALE    150		// this many ticks per second, really
	
	m_ActLED.Toggle();

	#if OUTPUT_SCREEN
		int nCount = 0;
	#endif
	
	int done = 0;
	unsigned int c = 0;
	unsigned int last_time = 0;
	unsigned int start_time = m_Timer.GetTicks();
	
	#if WITH_HTTP
		bool http_connected = false;
	#endif
	#if WITH_TFTP
		bool tftp_connected = false;
	#endif
	
	while (!done)
	{
		unsigned int now = m_Timer.GetTicks();
		if (now != last_time)
		{
			if (!last_time)
			{
				m_Logger.Write(logBoot, LogNotice,
					"Press <enter> to boot %s immediately",KERNEL_FILENAME);
				m_Logger.Write(logBoot, LogNotice,
					"Press <space> within %d seconds to upload file",REBOOT_TIME);
				m_ActLED.Toggle();
			}
			last_time = now;

			if (IsChainBootEnabled())
			{
				m_Logger.Write(logBoot, LogNotice,"external chain boot enabled ...");
				done = 1;
			}

			#if WITH_TFTP
				else if (pTFTP->client_connected && !tftp_connected)
				{
					tftp_connected = true;
					m_Logger.Write(logBoot, LogNotice,"TFTP Client connected");
				}
				else if (tftp_connected)	// stop looking for keystroks, don't timeout
				{
					m_Scheduler.Yield();
				}
			#endif
			#if WITH_HTTP
				else if (pHTTP->client_connected && !http_connected)
				{
					http_connected = true;
					m_Logger.Write(logBoot, LogNotice,"HTTP Client connected");
				}
				else if (http_connected)	// stop looking for keystroks, don't timeout
				{
					m_Scheduler.Yield();
				}
			#endif
			
			else if (m_pUseSerial->Read(&c,1))
			{
				if (c == ' ')
				{
					m_Logger.Write(logBoot, LogNotice,"Upload file ...");
					readBinarySerial();
					m_Logger.Write(logBoot, LogNotice,"back from upload IsChainBootEnabled=%d",IsChainBootEnabled());
					start_time =  m_Timer.GetTicks();
					last_time = 0;
					done = IsChainBootEnabled();
				}
				else if (c == 13 || c == 10)
				{
					m_Logger.Write(logBoot, LogNotice,"chain booting from SDCard now ...");
					done = 1;
				}
			}
			else if (now > start_time + (TIME_SCALE * REBOOT_TIME))
			{
				m_Logger.Write(logBoot, LogNotice,"timed out ...");
				done = 1;
			}
			else
			{
				#if OUTPUT_SCREEN
					nCount++;
					m_Screen.Rotor(0, nCount);
				#else
					if (now % TIME_SCALE == 0)
					{
						m_ActLED.Toggle();
						printf(".");
					}
				#endif
				
				#if WITH_HTTP || WITH_TFTP
					m_Scheduler.Yield();
				#endif
			}						
		}		
	}
}


//------------------------------------
// my binary upload protocol
//------------------------------------

#define logUpload  "binary"
#define RCV_TIMEOUT  500		
#define BS_BLOCKSIZE 2048

static const u32 BS_ACK = 'k';
static const u32 BS_NAK = 'n';
static const u32 BS_QUIT = 'q';


bool CKernel::read32Serial(u32 *retval)
{
	*retval = 0;
	int got = 0;
	u32 start = m_Timer.GetTicks();
	while (got < 4)
	{
		u32 ra = 0;
		if (m_pUseSerial->Read(&ra,1))
		{
			ra &= 0xff;
			*retval <<= 8;
			*retval |= ra;
			got++;
			start = m_Timer.GetTicks();				
		}
		else if (m_Timer.GetTicks() > start + RCV_TIMEOUT)
			return false;
	}
	return true;	
}


void CKernel::readBinarySerial()
	// came very close with initial implementation that just wrote
	// the length, the bytes, and a checksum, but feel that I need
	// a protocol, so here it is.
	//
	// To start we receive the length of the file as a 32 bit word.
	// If it's reasonable, we return ACK, otherwise QUIT
	//
	// The then receive a null terminated filename, a 32 bit mod_time
	// and create_time, and ACK or QUIT again
	//
	// We determine the number of blocks, including the last partial one, from the length.
	//
	// Each block is preceded by a 32 bit block number.   If it doesn't match
	// what we expect, we return QUIT.  If we timeout waiting for it, we QUIT.
	//
	// We receive the data bytes directly into our buffer, adding them up for a part
	// checksum. The checksum is an additional 32 bit number that follows the block.
	//
	//        4 byte block number --- 2048 byte block --- 4 byte checksum
	//
	// If we timeout during the bytes or checksum, or if the checksum does not match,
	// we NAK. Otherwise, we ACK and move to the next block.
	//
	// If partial, the last block is truncated, only the actual bytes and checksum are sent.
	//
	// After the we send the last block ack, we get an overal 32 bit checksum.
	// If it does not match we QUIT, otherwise, we ACK one more time.
{
	m_ActLED.Toggle();
	printf("waiting for binary file\r\n");
	
	// 1. read the length string
	
	u32 length = 0;
	if (!read32Serial(&length))
	{
		m_pUseSerial->Write(&BS_QUIT,1);
		printf("ERROR - timed out waiting for length\r\n");
		return;
	}
	// printf("got length = %d",length);
	if (length >= KERNEL_MAX_SIZE)
	{
		m_pUseSerial->Write(&BS_QUIT,1);
		printf("ERROR - length %d is > KERNEL_MAX_SIZE %d\r\n",length,KERNEL_MAX_SIZE);
		return;
	}
	if (length < 1024)
	{
		m_pUseSerial->Write(&BS_QUIT,1);
		printf("ERROR - length %d is < 1024 (probably bogus)\r\n",length);
		return;
	}
	
	// 2. allocate the buffer and ACK the length

	m_pKernelBuffer = new u8[length];
	if (!m_pKernelBuffer)
	{
		m_pUseSerial->Write(&BS_QUIT,1);
		printf("ERROR - could not allocate %d bytes\r\n",length);
		return;
	}
	m_pUseSerial->Write(&BS_ACK,1);
	
	// 2b. get the filename, mod_time, and create_time
	
	bool got_name = 0;
	u32 byte_index = 0;
	u32 start = m_Timer.GetTicks();
	while (!got_name)
	{
		u32 ra = 0;
		if (m_pUseSerial->Read(&ra,1))
		{
			ra &= 0xff;
			m_pKernelBuffer[byte_index++] = ra;
			start = m_Timer.GetTicks();
			if (!ra)
			{
				got_name = 1;
				break;
			}
		}
		else if (m_Timer.GetTicks() > start + RCV_TIMEOUT)
		{
			m_pUseSerial->Write(&BS_QUIT,1);
			printf("ERROR - timed out waiting for filename\r\n");
			return;
		}
	}

	m_kernel_filename = (const char *) m_pKernelBuffer;
	if (!read32Serial(&m_kernel_mod_time))
	{
		m_pUseSerial->Write(&BS_QUIT,1);
		printf("ERROR - timed out waiting for mod_time\r\n");
		return;
	}
	if (!read32Serial(&m_kernel_create_time))
	{
		m_pUseSerial->Write(&BS_QUIT,1);
		printf("ERROR - timed out waiting for create_time\r\n");
		return;
	}
	m_pUseSerial->Write(&BS_ACK,1);
	printf("got filename %s\r\n",m_pKernelBuffer);
	printf("mod_time=%d\r\n",m_kernel_mod_time);
	printf("create_time=%d\r\n",m_kernel_mod_time);
	
	// ready to start receiving blocks
	
	u32 addr = 0;		// next (block) address to write to
	u32 block = 0;		// the next block we are expecting
	u32 total_sum = 0;	// overall checksum
	u32 num_blocks = (length + BS_BLOCKSIZE - 1) / BS_BLOCKSIZE;
	while (block < num_blocks)
	{
		
redo_block:
		// 3. read a binary block_num
		// quit on timeout or if it's not what we expect
		
		m_ActLED.Toggle();
		
		u32 block_num = 0;
		if (!read32Serial(&block_num))
		{
			m_pUseSerial->Write(&BS_QUIT,1);
			printf("ERROR - timed out waiting for block_num(%d)\r\n",block);
			free(m_pKernelBuffer);
			return;
		}
		if (block_num != block)
		{
			m_pUseSerial->Write(&BS_QUIT,1);
			printf("ERROR - got block_num(%d) expected(%d)\r\n",block_num,block);
			free(m_pKernelBuffer);
			return;
		}
		m_pUseSerial->Write(&BS_ACK,1);

		// 4. read the bytes
		
		u32 part_sum = 0;
		u32 byte_index = 0;
		u32 temp_sum = total_sum;
		u32 left = length - addr;
		if (left > BS_BLOCKSIZE)
			left = BS_BLOCKSIZE;
		u32 start = m_Timer.GetTicks();

		while (byte_index < left)
		{
			u32 ra = 0;
			if (m_pUseSerial->Read(&ra,1))
			{
				ra &= 0xff;
				part_sum += ra;
				temp_sum += ra;
				m_pKernelBuffer[addr + byte_index++] = ra;
				start = m_Timer.GetTicks();
			}
			else if (m_Timer.GetTicks() > start + RCV_TIMEOUT)
			{
				m_pUseSerial->Write(&BS_NAK,1);
				printf("ERROR - timed out waiting for data\r\n");
				goto redo_block;
			}
		}
		
		// 4b. and verify the checksum
		
		u32 got_sum = 0;
		if (!read32Serial(&got_sum))
		{
			m_pUseSerial->Write(&BS_NAK,1);
			printf("ERROR - timed out waiting for part_sum\r\n");
			goto redo_block;
		}
		if (got_sum != part_sum)
		{
			m_pUseSerial->Write(&BS_NAK,1);
			printf("ERROR -got_sum(%d) != calculated(%d)\r\n",got_sum,part_sum);
			goto redo_block;
		}

		// 5. finished with that block
		// bump things and ACK
		
		m_pUseSerial->Write(&BS_ACK,1);
		block++;
		addr += left;	// BS_BLOCKSIZE
		total_sum = temp_sum;

	}	// while (block<num_blocks)			
		
	// 6. read the overall checksum (hex asci)

	u32 check = 0;
	if (!read32Serial(&check))
	{
		m_pUseSerial->Write(&BS_QUIT,1);
		printf("ERROR - timed out waiting for final checksum(%d)\r\n",total_sum);
		free(m_pKernelBuffer);
		return;
	}
	if (total_sum != check)
	{
		m_pUseSerial->Write(&BS_QUIT,1);
		printf("ERROR - final checksum(%d) does not match calculated(%d)\r\n",check,total_sum);
		free(m_pKernelBuffer);
		return;
	}
	
	// otherwise, we're good to reboot to it ...
	
	m_kernel_size = length;	
	printf("OK!! enabling chain boot for binary(%d) sum=%d\r\n",m_kernel_size,total_sum);
	EnableChainBoot(m_pKernelBuffer, m_kernel_size);
}




//----------------------------------------------
// Run()
//----------------------------------------------

TShutdownMode CKernel::Run(void)
{
	m_ActLED.Toggle();
	m_Logger.Write(logBoot, LogNotice, "PRH bootloader version 1.07");
	m_Logger.Write(logBoot, LogNotice, "Compile time: " __DATE__ " " __TIME__);
	m_ActLED.Toggle();
	
	// debugDumpEnvironment();	
	// m_ActLED.Toggle();

	// Mount file system and find the kernel
	
	#if WITH_FILE
		initFileSystem();
	#endif
	
	// start HTTP and/or TFTP server(s)
	
	#if WITH_HTTP || WITH_TFTP
		CString IPString;
		m_Net.GetConfig()->GetIPAddress()->Format(&IPString);
		#if WITH_HTTP
			pHTTP = new CHTTPBootServer(&m_Net, HTTP_BOOT_PORT, KERNEL_MAX_SIZE + 2000);
			m_ActLED.Toggle();
			m_Logger.Write(logBoot, LogNotice,
				"Open \"http://%s:%u/\" in your web browser!",
				(const char *) IPString, HTTP_BOOT_PORT);
		#endif
		#if WITH_TFTP
			pTFTP = new CTFTPBootServer(&m_Net, KERNEL_MAX_SIZE);
			m_ActLED.Toggle();
			m_Logger.Write(logBoot, LogNotice,
				"Try \"tftp -m binary %s -c put kernel.img\" from another computer!",
				(const char *) IPString);
		#endif
	#endif

	// wait for an upload or fall thru on timeout
	
	waitForUpload();

	// if we obtained a new kernel above,
	// write it out to the disk
	
	#if WITH_FILE && WRITE_KERNEL
		if (IsChainBootEnabled())
			writeKernelToSD();
	#endif
	
	// if no new kernel, read the kernel from the SD card
	
	#if WITH_FILE 
		if (m_kernel_size && !IsChainBootEnabled())
			readKernelFromSD();
		closeFileSystem();
	#endif
	
	// boot the kernel (or reboot if none exists)
	// Note that existence of m_Scheduler breaks chain booting
	
	m_Logger.Initialize(0);
	m_pUseSerial->Write("\r\n\r\n",4);
	m_ActLED.Blink(10,20,20);
	m_Timer.MsDelay(500);
	
	// m_Scheduler.Sleep(2);
	
	return ShutdownReboot;

}	// kernel(bootloader).Run()




void CKernel::debugDumpEnvironment()
	// debugging to show stuff from m_Options, and the
	// the raw, unparsed kernel command line ..
{
	m_Logger.Write(logBoot, LogNotice, "screen(%d,%d)",
		m_Options.GetWidth(),
		m_Options.GetHeight());
	if (m_Options.GetLogDevice())
		m_Logger.Write(logBoot, LogNotice, "log_device(%s)",
			m_Options.GetLogDevice());
	m_Logger.Write(logBoot, LogNotice, "log_level(%d)",
		m_Options.GetLogLevel());
	if (m_Options.GetKeyMap())
		m_Logger.Write(logBoot, LogNotice, "keymap(%s)",
			m_Options.GetKeyMap());
	m_Logger.Write(logBoot, LogNotice, "usb_power_delay(%d)",
		m_Options.GetUSBPowerDelay());
	if (m_Options.GetSoundDevice())
		m_Logger.Write(logBoot, LogNotice, "sound_device(%s)",
			m_Options.GetSoundDevice());
	m_Logger.Write(logBoot, LogNotice, "sound_option(%d)",
		m_Options.GetSoundOption());
	m_Logger.Write(logBoot, LogNotice, "cpu_speed(%d)",
		m_Options.GetCPUSpeed());
	m_Logger.Write(logBoot, LogNotice, "max_temp(%d)",
		m_Options.GetSoCMaxTemp());
	
	CBcmPropertyTags Tags;
	TPropertyTagCommandLine m_TagCommandLine;	
	if (Tags.GetTag(PROPTAG_GET_COMMAND_LINE, &m_TagCommandLine, sizeof m_TagCommandLine))
	{
		char *p = (char *) m_TagCommandLine.String;
		while (*p)
		{
			const char *opt = p;
			while (*p && *p != ' ')
			{
				p++;
			}
			if (*p == ' ')
			{
				*p++ = 0;
			}
			printf("    part=%s\r\n",opt);
		}
	}
	
	m_CPUThrottle.DumpStatus(true);
						  
}

