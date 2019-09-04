// NOT WORKING
// See comments and reworked defines in my Circle/miniUart.cpp


#include "_hci_defs.h"		// for LOG()
#include "transportMiniUart.h"
#include <circle/devicenameservice.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/logger.h>

#define log_name 		"bttmini"



//-------------------------------------------------------------------------
// from Dave Welch's bootloader
//-------------------------------------------------------------------------

#define PBASE 			ARM_IO_BASE

// #define GPFSEL1         (PBASE+0x00200004)
	// original define, at 10 gpios per register
	// pins 13,14 were in register #1
#define GPFSEL4         (PBASE+0x00200010)
	// prh added
	// pin 32,33 are in register #4


#define GPPUD           (PBASE+0x00200094)
#define GPPUDCLK0       (PBASE+0x00200098)

#define AUX_ENABLES     (PBASE+0x00215004)
#define AUX_MU_IO_REG   (PBASE+0x00215040)
#define AUX_MU_IER_REG  (PBASE+0x00215044)
#define AUX_MU_IIR_REG  (PBASE+0x00215048)
#define AUX_MU_LCR_REG  (PBASE+0x0021504C)
#define AUX_MU_MCR_REG  (PBASE+0x00215050)
#define AUX_MU_LSR_REG  (PBASE+0x00215054)
#define AUX_MU_MSR_REG  (PBASE+0x00215058)
#define AUX_MU_SCRATCH  (PBASE+0x0021505C)
#define AUX_MU_CNTL_REG (PBASE+0x00215060)
#define AUX_MU_STAT_REG (PBASE+0x00215064)
#define AUX_MU_BAUD_REG (PBASE+0x00215068)



//======================================================================
// Attempt to add polling task() did not work
//======================================================================

#include <circle/sched/task.h>
#include <circle/sched/scheduler.h>

class miniTask : public CTask
{
public:
    
	miniTask(transportMiniUart *pMini)
	{
		m_pMini = pMini;
	}
	~miniTask(void)
	{
		m_pMini = 0;
	}

	void Run(void)
	{
		while (1)
		{	
			if (read32(AUX_MU_LSR_REG)&0x01)
			{
				u8 nData = read32(AUX_MU_IO_REG) & 0xFF;
				m_pMini->Receive(nData);
			}
			CScheduler::Get()->Yield();
		}

	}

private:
    
	transportMiniUart *m_pMini;
    
};



//-------------------------------------------------------------------------



transportMiniUart::transportMiniUart(CInterruptSystem *pInterruptSystem) :
	m_pInterruptSystem(pInterruptSystem)
{
	m_bIRQConnected = false;
}

transportMiniUart::~transportMiniUart (void)
{
	if (m_bIRQConnected)
	{
		assert(m_pInterruptSystem != 0);
		m_pInterruptSystem->DisconnectIRQ(ARM_IRQ_AUX);
	}
	m_bIRQConnected = false;
	m_pInterruptSystem = 0;
}


void transportMiniUart::initialize (unsigned nBaudrate /*= 115200 */)
{
	LOG("transportMiniUart::initialize(%d)",nBaudrate);
	
	//-------------------------------
	// setup the pins first
	//-------------------------------
	// gpio32 is the 3rd pin in the register,
	// so is shifted 0,3,6 bits .. we use also
	// use alt5, which is binary 2
	
	volatile unsigned int ra=read32(GPFSEL4);
	ra &= ~(7<<6); 		// gpio32 (mask not needed)
	ra |=  (2<<6); 		// alt5
	ra &= ~(7<<9); 		// gpio33 (mask not needed)
	ra |=  (2<<9); 		// alt5
	write32(GPFSEL4,ra);
	write32(GPPUD,0);	// no pulldowns
		
	// original code for pins 14,15	
	// each pin gets 3 bits in the regsister
	// so gpio14, which is the 5th pin ia
	// shifted 0,3,6,9,12 bits ... 
	//	  
	//	  volatile unsigned int ra=read32(GPFSEL1);
	//	  ra&=~(7<<12); 		// gpio14
	//	  ra|=2<<12;    		// alt5
	//	  ra&=~(7<<15); 		// gpio15
	//	  ra|=2<<15;    		// alt5
	//	  write32(GPFSEL1,ra);
	
	//-------------------------------
	// setup the clock 
	//-------------------------------
	
	#if 0	// doesn't work
		for(ra=0;ra<150;ra++) ra=0;	// dummy(ra);
		write32(GPPUDCLK0,(1<<14)|(1<<15));
		for(ra=0;ra<150;ra++) ra=0;	// dummy(ra);
		write32(GPPUDCLK0,0);
	#endif

	write32(AUX_MU_IER_REG,0);			// enable receive interrupts
	write32(AUX_MU_CNTL_REG,0); 			// disable mini uart xmit and receive
	write32(AUX_MU_LCR_REG,1);	// 3);	// 0x1=8 bit mode, 0x2=reserved ... should be zero
	write32(AUX_MU_MCR_REG,0);			// RTS is active low
	write32(AUX_MU_IER_REG,0);			// clear the interrups? (seems like youre supposed to write a 1 in the doc)
	write32(AUX_MU_IIR_REG,0xC6);			// bit1 (0x02) generate receive interrupts
	write32(AUX_MU_BAUD_REG,(250000000/8 + nBaudrate/2) / nBaudrate - 1);


	//-------------------------------
	// attach the interrupt
	//-------------------------------
	// also not working in miniUart.cpp
	
	#if 1
		// create a scheduler task to poll the port
		new miniTask(this);
	#endif
	
	assert(m_pInterruptSystem != 0);
	m_pInterruptSystem->ConnectIRQ(ARM_IRQ_AUX, IRQStub, this);
	m_bIRQConnected = TRUE;

	
	//-------------------------------
	// start it up
	//-------------------------------
	// and nothing happens ...
	
	write32(AUX_MU_CNTL_REG,3);			// enable mini-uart xmit and receive
	write32(AUX_ENABLES,1);				// enable the mini_uart (as opposed to SPI)
										// should be done last
	
	CDeviceNameService::Get ()->AddDevice ("ttyUBT2", this, FALSE);
	LOG("transportMiniUart::initialize() finished",0);
}



void transportMiniUart::Write(u8 nChar)
{
	printf("mini_uart_sending: 0x%02x\n",nChar);
	while(1)
	{
		if(read32(AUX_MU_LSR_REG)&0x20) break;
	}
	write32(AUX_MU_IO_REG,nChar);
	
	// flush

	#if 1
		while(1)
		{
			if((read32(AUX_MU_LSR_REG)&0x100)==0) break;
		}
	#endif
}



void transportMiniUart::IRQStub(void *pParam)
{
	printf("mini_uart IRQStub() called",0);
	transportMiniUart *pThis = (transportMiniUart *) pParam;
	assert(pThis != 0);
	pThis->IRQHandler();
}


void transportMiniUart::IRQHandler(void)
{
	PeripheralEntry();

	// PRH - handle buffer overlows
	//	u32 nMIS = read32(ARM_UART0_MIS);
	//	if (nMIS & INT_OE)
	//	{
	//		CLogger::Get()->Write(log_name, LogPanic, "Overrun error");
	//		write32(ARM_UART0_ICR, nMIS);
	//		PeripheralExit();
	//		return;
	//	}

	// PRH - clear the interrupt
	// write32(ARM_UART0_ICR, nMIS);
	
	// while (!(read32(ARM_UART0_FR) & FR_RXFE_MASK))
	while (read32(AUX_MU_LSR_REG) & 0x01)
	{
		// u8 nData = read32(ARM_UART0_DR) & 0xFF;
		u8 nData = read32(AUX_MU_IO_REG) & 0xFF;
		Receive(nData);
	}

	PeripheralExit();
}


