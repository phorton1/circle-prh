// bootloader kernel.cpp - prh
//
// NOTE: YOU MUST DO A CLEAN CIRCLE BUILD
//		> cd /src/circle
//		> makeall clean
//
// with "ARM_ALLOW_MULTI_CORE" commented out
// in /src/circle/include/circle/sysconfig.h
//
// As the bootloader uses circl hain loading
// which is only supported on single core builds.
//
//------------------------------------------------------
//
// this file is compiled and linked to recoveryN.img
//
// the standard stock raspberry pi bootcode.bin and
// start.elf scheme attempts to load recovery.img before
// kernel.img
//
// so by calling our program recovery.img, we can allow
// folks to create the expected kernel.img's that are common
// to most bare metal makefile projects, which we can then
// chain boot to.

#define SERIAL_BAUD_RATE	460800	// 115200

#define WRITE_KERNEL 		 1
#define SHOW_ROOT_DIRECTORY  0

#include "kernel.h"
#include <assert.h>
#include <circle/util.h>
#include <circle/alloc.h>
#include <circle/chainboot.h>
#include <circle/sysconfig.h>


// #ifdef ARM_ALLOW_MULTI_CORE
// 	#error The bootloader must be compiled single core
// #endif


#define logBoot 	"boot"

#if USE_CIRCLE_FAT
	#define PARTITION	"emmc1-1"
#else
	#define DRIVE		"SD:"
#endif


#if RASPPI == 3
	#define KERNEL_FILENAME	 "kernel8-32.img"
		// wont work - long filenames not supported in circle
#elif RASPPI == 2
	#define KERNEL_FILENAME  "kernel7.img"
#else
	#define KERNEL_FILENAME  "kernel.img"
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
	m_Logger(m_Options.GetLogLevel()),
	m_EMMC(&m_Interrupt, &m_Timer, &m_ActLED)
{
	m_pSerialDevice = 0;

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
			m_pSerialDebug = &m_Screen;
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
		bOK = m_Serial.Initialize(SERIAL_BAUD_RATE);
		m_pSerialDevice = &m_Serial;
		m_ActLED.Toggle();
		bOK = bOK && m_Logger.Initialize(&m_Serial);
		#if !WITH_SCREEN
			m_pSerialDebug = &m_Serial;
		#endif
		m_Logger.Write(logBoot, LogNotice,
			"serial baud_rate=%u",SERIAL_BAUD_RATE);
	}
	
	if (bOK)
	{
		bOK = m_EMMC.Initialize();
		m_ActLED.Toggle();
	}
		
	return bOK;
}


//----------------------------------------------
// file routines
//----------------------------------------------


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

	// READ THE FILE USING THE ADDON FAT FS

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
}


#if WRITE_KERNEL
	void CKernel::writeKernelToSD()
		// if we got a kernel above, save it
		// as determined by IsChainBootEnabled() at this point
	{
		m_Logger.Write(logBoot, LogDebug, "saving kernel %s", (const char *) m_kernel_filename);

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
	}	// writeKernelToSD()
#endif	// WRITE_KERNEL




//----------------------------------------
// memory routines
//----------------------------------------

void CKernel::waitForUpload()
{
	#define REBOOT_TIME   3			// just enough time for my perl script to see it
	#define TIME_SCALE    150		// this many ticks per second, really
	
	m_ActLED.Toggle();

	int done = 0;
	unsigned int c = 0;
	unsigned int last_time = 0;
	unsigned int start_time = m_Timer.GetTicks();
	
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

			
			else if (m_pSerialDevice->Read(&c,1))
			{
				if (c == ' ')
				{
					m_Logger.Write(logBoot, LogNotice,"Upload file ...");
						// last logger output before serial protocol
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

				if (now % TIME_SCALE == 0)
				{
					m_ActLED.Toggle();
				}
			}						
		}		
	}
}


//------------------------------------
// my binary upload protocol
//------------------------------------
// if WITH_SCREEN all debugging sent to the rPi screen and
// no debugging bytes are sent to the binary serial device,

void CKernel::dbg_serial(const char *pMessage, ...)
{
	if (m_pSerialDebug)
	{
		va_list var;
		va_start(var, pMessage);
		CString Message;
		Message.FormatV(pMessage, var);
		// unsigned int len1 = pDevice->Write("\x1b[96m",5);		// light cyan
		m_pSerialDebug->Write((const char *) Message, Message.GetLength() );
		va_end(var);
    }
}


#define DEBUG_PROTOCOL  0

#define RCV_TIMEOUT  1000	// 500
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
		if (m_pSerialDevice->Read(&ra,1))
		{
			#if DEBUG_PROTOCOL
				dbg_serial("bootloader read32[%d]=%d\r\n",got,ra);
			#endif
			
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
	dbg_serial("waiting for binary file\r\n");
	
	// 1. read the 4 byte length
	
	u32 length = 0;
	if (!read32Serial(&length))
	{
		m_pSerialDevice->Write(&BS_QUIT,1);
		dbg_serial("ERROR - timed out waiting for length\r\n");
		return;
	}
	if (length >= KERNEL_MAX_SIZE)
	{
		m_pSerialDevice->Write(&BS_QUIT,1);
		dbg_serial("ERROR - length %d is > KERNEL_MAX_SIZE %d\r\n",length,KERNEL_MAX_SIZE);
		return;
	}
	if (length < 1024)
	{
		m_pSerialDevice->Write(&BS_QUIT,1);
		dbg_serial("ERROR - length %d is < 1024 (probably bogus)\r\n",length);
		return;
	}
	
	// 2. allocate the buffer and ACK the length

	m_pKernelBuffer = new u8[length];
	if (!m_pKernelBuffer)
	{
		m_pSerialDevice->Write(&BS_QUIT,1);
		dbg_serial("ERROR - could not allocate %d bytes\r\n",length);
		return;
	}
	m_pSerialDevice->Write(&BS_ACK,1);
	dbg_serial("got length = %d\r\n",length);
	
	// 2b. get the filename, mod_time, and create_time
	
	bool got_name = 0;
	u32 byte_index = 0;
	u32 start = m_Timer.GetTicks();
	while (!got_name)
	{
		u32 ra = 0;
		if (m_pSerialDevice->Read(&ra,1))
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
			m_pSerialDevice->Write(&BS_QUIT,1);
			dbg_serial("ERROR - timed out waiting for filename\r\n");
			return;
		}
	}

	m_kernel_filename = (const char *) m_pKernelBuffer;
	if (!read32Serial(&m_kernel_mod_time))
	{
		m_pSerialDevice->Write(&BS_QUIT,1);
		dbg_serial("ERROR - timed out waiting for mod_time\r\n");
		return;
	}
	if (!read32Serial(&m_kernel_create_time))
	{
		m_pSerialDevice->Write(&BS_QUIT,1);
		dbg_serial("ERROR - timed out waiting for create_time\r\n");
		return;
	}
	m_pSerialDevice->Write(&BS_ACK,1);
	dbg_serial("got filename %s\r\nmod_time=%u\r\ncreate_time=%u\r\n",
		m_pKernelBuffer,
		m_kernel_mod_time,
		m_kernel_create_time);
	
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
			m_pSerialDevice->Write(&BS_QUIT,1);
			dbg_serial("ERROR - timed out waiting for block_num(%d)\r\n",block);
			free(m_pKernelBuffer);
			return;
		}
		if (block_num != block)
		{
			m_pSerialDevice->Write(&BS_QUIT,1);
			dbg_serial("ERROR - got block_num(%d) expected(%d)\r\n",block_num,block);
			free(m_pKernelBuffer);
			return;
		}
		m_pSerialDevice->Write(&BS_ACK,1);

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
			if (m_pSerialDevice->Read(&ra,1))
			{
				ra &= 0xff;
				part_sum += ra;
				temp_sum += ra;
				m_pKernelBuffer[addr + byte_index++] = ra;
				start = m_Timer.GetTicks();
			}
			else if (m_Timer.GetTicks() > start + RCV_TIMEOUT)
			{
				m_pSerialDevice->Write(&BS_NAK,1);
				dbg_serial("ERROR - timed out waiting for data\r\n");
				goto redo_block;
			}
		}
		
		// 4b. and verify the checksum
		
		u32 got_sum = 0;
		if (!read32Serial(&got_sum))
		{
			m_pSerialDevice->Write(&BS_NAK,1);
			dbg_serial("ERROR - timed out waiting for part_sum\r\n");
			goto redo_block;
		}
		if (got_sum != part_sum)
		{
			m_pSerialDevice->Write(&BS_NAK,1);
			dbg_serial("ERROR -got_sum(%d) != calculated(%d)\r\n",got_sum,part_sum);
			goto redo_block;
		}

		// 5. finished with that block
		// bump things and ACK
		
		m_pSerialDevice->Write(&BS_ACK,1);
		block++;
		addr += left;	// BS_BLOCKSIZE
		total_sum = temp_sum;

	}	// while (block<num_blocks)			
		
	// 6. read the overall checksum (hex asci)

	u32 check = 0;
	if (!read32Serial(&check))
	{
		m_pSerialDevice->Write(&BS_QUIT,1);
		dbg_serial("ERROR - timed out waiting for final checksum(%d)\r\n",total_sum);
		free(m_pKernelBuffer);
		return;
	}
	if (total_sum != check)
	{
		m_pSerialDevice->Write(&BS_QUIT,1);
		dbg_serial("ERROR - final checksum(%d) does not match calculated(%d)\r\n",check,total_sum);
		free(m_pKernelBuffer);
		return;
	}
	
	// send ACK for checksum
	
	m_pSerialDevice->Write(&BS_ACK,1);

	// otherwise, we're good to reboot to it ...
	
	m_kernel_size = length;
	dbg_serial("OK!! enabling chain boot for binary(%d) sum=%d\r\n",m_kernel_size,total_sum);
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

	// Mount file system and find the kernel
	
	initFileSystem();

	// wait for an upload or fall thru on timeout
	
	waitForUpload();

	// if we obtained a new kernel above,
	// write it out to the disk
	
	#if WRITE_KERNEL
		if (IsChainBootEnabled())
			writeKernelToSD();
	#endif
	
	// if no new kernel, read the kernel from the SD card
	
	if (m_kernel_size && !IsChainBootEnabled())
		readKernelFromSD();
	closeFileSystem();
	
	// boot the kernel (or reboot if none exists)
	// Note that existence of m_Scheduler breaks chain booting
	
	m_Logger.Initialize(0);
	m_pSerialDevice->Write("\r\n\r\n",4);
	m_ActLED.Blink(10,20,20);
	m_Timer.MsDelay(500);
	
	// m_Scheduler.Sleep(2);
	
	return ShutdownReboot;

}	// kernel(bootloader).Run()





