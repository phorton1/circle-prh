// kernel.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>


#include "kernel.h"
#include <assert.h>
#include <circle/util.h>
#include <circle/machineinfo.h>
#include <circle/devicenameservice.h>
#include "btTester.h"

#if HCI_USE_FAT_DATA_FILE
	#define DRIVE		"SD:"
#endif


static const char log_name[] = "kernel";


CKernel::CKernel(void) :
	#if USE_SCREEN
		m_Screen(m_Options.GetWidth(), m_Options.GetHeight()),
	#endif
	#if USE_MINI_SERIAL
		m_MiniUart(&m_Interrupt),
	#endif
	m_Timer(&m_Interrupt),
	#if USE_MAIN_SERIAL
		m_Serial(&m_Interrupt, TRUE),
	#endif
	m_Logger(m_Options.GetLogLevel(), &m_Timer)
	#if USE_USB
		,m_DWHCI(&m_Interrupt, &m_Timer)
	#endif
	#if HCI_USE_FAT_DATA_FILE
		,m_EMMC(&m_Interrupt, &m_Timer, &m_ActLED)
	#endif
	
{
	m_ActLED.Toggle();
}


CKernel::~CKernel (void)
{
	#if HCI_USE_FAT_DATA_FILE
		// Unmount file system
		if (f_mount (0, DRIVE, 0) != FR_OK)
			LOG_ERROR("Cannot unmount drive: %s", DRIVE);
	#endif
}



boolean CKernel::Initialize (void)
	// we presume that if you have a serial port AND
	// the screen, that you want logging/output to go
	// to the serial port, and that the screen is for UI.
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
	
	#if HCI_USE_FAT_DATA_FILE
		if (bOK)
			bOK = m_EMMC.Initialize();
		if (bOK)
		{
			if (f_mount(&m_FileSystem, DRIVE, 1) != FR_OK)
			{
				LOG_ERROR("Cannot mount drive: %s", DRIVE);
				bOK = false;
			}
		}
	#endif
	
	#if USE_USB
		if (bOK)
			bOK = m_DWHCI.Initialize();
	#endif
	
	if (bOK)
	{
		// in my system the client decides which transport (and hence controller) to use	
		
		CDeviceNameService *ns = CDeviceNameService::Get();
		CUSBBluetoothDevice *pUSB = (CUSBBluetoothDevice *) ns->GetDevice("ubt1", FALSE);
		
		if (pUSB)
		{
			LOG("using USB transport",0);
			bOK = m_bt.Initialize(
				(btTransportBase *) pUSB
				#if HCI_USE_FAT_DATA_FILE
					,&m_FileSystem
				#endif
			);
		}
		else
		//		if (ns->GetDevice("ttyS1", FALSE) == 0 && (
		//			 mi->GetMachineModel() == MachineModel3B ||
		//			 mi->GetMachineModel() == MachineModel3BPlus ||
		//			 mi->GetMachineModel() == MachineModelZeroW))
		{
			#if USE_MINI_SERIAL
				LOG("using UART transport",0);
				transportUart *pUART = new transportUart(&m_Interrupt);
				assert(pUART != 0);
				pUART->initialize();
			#else
				LOG("using MINI_UART transport",0);
				transportMiniUart *pUART = new transportMiniUart(&m_Interrupt);
				assert(pUART != 0);
				pUART->initialize();
			#endif
			
			bOK = m_bt.Initialize(pUART
				#if HCI_USE_FAT_DATA_FILE
					,&m_FileSystem
				#endif
			);
		}	
	}

	return bOK;
}





TShutdownMode CKernel::Run(void)
{
	m_Logger.Write(log_name, LogNotice, "btTest " __DATE__ " " __TIME__);

	#if USE_MINI_SERIAL
		btTester tester(&m_MiniUart,&m_bt);
	#endif
	#if USE_MAIN_SERIAL
		btTester tester(&m_Serial,&m_bt);
	#endif
	
	printf("ready ...\n");
	m_Scheduler.MsSleep(400);
	CGPIOPin toTeensy(25,GPIOModeOutput);
	toTeensy.Write(1);
	
	//-----------------------------
	// The loop
	//-----------------------------

	while (1)
	{
		tester.task();

		#define BLINK_TIME 40	
		static u32 blink_time = 0;
		u32 now = m_Timer.GetTicks();
		if (now > blink_time + BLINK_TIME)
		{
			m_ActLED.Toggle();
			blink_time = now;
		}
	}	
	
	return ShutdownHalt;

}	// CKernel::Run()


