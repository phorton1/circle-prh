
#include "kernel.h"

static const char FromKernel[] = "kernel";

CKernel *CKernel::global_kernel = 0;


CKernel::CKernel(void) :
#if WITH_SCREEN
	m_Screen(m_Options.GetWidth(), m_Options.GetHeight()),
#endif
	m_Timer(&m_Interrupt),
	m_Serial(&m_Interrupt, TRUE),
	m_Logger(m_Options.GetLogLevel(), &m_Timer)
{
	m_ActLED.Toggle();
	global_kernel = this;
}


CKernel::~CKernel(void)
{
	global_kernel = 0;
}


boolean CKernel::Initialize(void)
{
	boolean bOK = TRUE;
	m_ActLED.Toggle();

#if WITH_SCREEN
	if (bOK)
	{
		bOK = m_Screen.Initialize();
		m_ActLED.Toggle();
	}
	if (bOK)
	{
		bOK = m_Logger.Initialize(&m_Screen);
		m_ActLED.Toggle();
	}
#endif

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
		bOK = m_Serial.Initialize(115200);	 // 921600);	 // 115200);
		m_ActLED.Toggle();
	}
	
	if (bOK)
	{
		bOK = m_Logger.Initialize(&m_Serial);
		m_ActLED.Toggle();
	}

	return bOK;
}



TShutdownMode CKernel::Run(void)
{
	m_Logger.Write(FromKernel, LogNotice, "prh usb_device test program v1.00 " __DATE__ " " __TIME__);
	usbStuff(0);	// init

	printf("waiting\r\n");

	u32 tm = m_Timer.GetTicks();
	while (1)
	{
		usbStuff(1);	// task
		
		unsigned int c;
		if (m_Serial.Read(&c,1))
		{
			c &= 0xff;
			if (c >= 'a' && c <= 'z')
			{
				usbStuff(c - 'a' + 2);
			}
			else
			{
				m_Serial.Write(&c,1);
			}
			m_ActLED.Toggle();
		}
		
		// the timer is off by about a factor of 4 (too slow)
		else if (m_Timer.GetTicks() > tm + 250)
		{
			m_ActLED.Toggle();
			tm = m_Timer.GetTicks();			
		}
	}
	
	return ShutdownHalt;
}




//----------------------------------
// stdio
//----------------------------------

void printf(const char *pMessage, ...)
{
	CDevice *pDevice = CKernel::Get()->getOutputDevice();
	if (pDevice)
	{
		va_list var;
		va_start(var, pMessage);
		CString Message;
		Message.FormatV(pMessage, var);
		pDevice->Write((const char *) Message, Message.GetLength());
		va_end(var);
	}
}