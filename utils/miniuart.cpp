//
// miniuart.cpp
//
// A serial device based on dwelch's mini-uart code
// ripped from circle/tools/bootloarder.
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
//

#include "miniuart.h"
#include "utils/myUtils.h"
#include <circle/devicenameservice.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/logger.h>
	
//-------------------------------------------------------------------------
// from Dave Welch's bootloader
//-------------------------------------------------------------------------
// duplicated defines from Circle
//
// #define GPFSEL1         	(ARM_IO_BASE + 0x00200004)
// #define GPSET0          	(ARM_IO_BASE + 0x0020001C)
// #define GPCLR0          	(ARM_IO_BASE + 0x00200028)
// #define GPPUD           	(ARM_IO_BASE + 0x00200094)
// #define GPPUDCLK0       	(ARM_IO_BASE + 0x00200098)
// #define AUX_ENABLES     	(ARM_IO_BASE + 0x00215004)

// these defines are missing in Circle bcm2835.h and
// so are duplicated here and in my other code

#define AUX_MU_IO_REG   	(ARM_IO_BASE + 0x00215040)
#define AUX_MU_IER_REG  	(ARM_IO_BASE + 0x00215044)
#define AUX_MU_IIR_REG  	(ARM_IO_BASE + 0x00215048)
#define AUX_MU_LCR_REG  	(ARM_IO_BASE + 0x0021504C)
#define AUX_MU_MCR_REG  	(ARM_IO_BASE + 0x00215050)
#define AUX_MU_LSR_REG  	(ARM_IO_BASE + 0x00215054)
#define AUX_MU_MSR_REG  	(ARM_IO_BASE + 0x00215058)
#define AUX_MU_SCRATCH  	(ARM_IO_BASE + 0x0021505C)
#define AUX_MU_CNTL_REG 	(ARM_IO_BASE + 0x00215060)
#define AUX_MU_STAT_REG 	(ARM_IO_BASE + 0x00215064)
#define AUX_MU_BAUD_REG 	(ARM_IO_BASE + 0x00215068)



CMiniUartDevice::CMiniUartDevice(CInterruptSystem *pInterruptSystem) :
	m_pInterruptSystem(pInterruptSystem),
	m_bInterruptConnected(0),
    m_nRxInPtr(0),
    m_nRxOutPtr(0),
    m_nRxStatus(0)
{
}


CMiniUartDevice::~CMiniUartDevice (void)
{
	if (m_bInterruptConnected)
	{
		assert(m_pInterruptSystem != 0);
		m_pInterruptSystem->DisconnectIRQ(ARM_IRQ_AUX);
	}
	m_bInterruptConnected = false;
	m_pInterruptSystem = 0;	
}



boolean CMiniUartDevice::Initialize (unsigned nBaudrate /*= 115200 */)
{
	CLogger::Get()->Write("miniuart",LogDebug,"CMiniUartDevice::Initialize(%d)",nBaudrate);
	PeripheralEntry();			

	// doc says to setup the pins before configuring the uart ...

	volatile unsigned int ra=read32(ARM_GPIO_GPFSEL1);
	ra &= ~(7<<12);		// gpio14
	ra |=  (2<<12);    	// alt5
	ra &= ~(7<<15); 	// gpio15
	ra |=  (2<<15);    	// alt5
	write32(ARM_GPIO_GPFSEL1,ra);
	write32(ARM_GPIO_GPPUD,0);
		
	// Note Documentation errors:
	//
	// (a) that the titles of the IER and IIR registers are mixed up
	//     in the documentation.  The synopsis has the correct names.
	//     IER comes before IIR ...
	//
	// (b) furthermore, the errata mysteriously says that IER "Bits
	//     3:2 are marked as don't care, but are actually required,
	//     in order to receive interrupts" and that "Bits 1:0 are
	//     [sic]swaped. bit 0 is receive interrupt and bit 1 is transmit."
	//
	//     .. setting the mysterious bits doesn't help and they dont seem to stick
	//
	// (c) 	I suspect the doc is wrong about bits in the STAT register
	//      too, although nothing about this is mentioned in the errata.
	//
	//      (c1) Bit 4 (0x10) is documented as "5 Transmit FIFO is full",
	//           yet in my testing it actually gets set when the RECIEVE fifo
	//           is full ..
	//
	//      (c2) same thing goes for the "idle" bits, as bit 3 (0x4) is toggled
	// 			 off briefly when I am receiving something. My program is not
	//           sending anything.
	// 
	
	write32(ARM_AUX_ENABLE,1);		// enable the mini uart
	write32(AUX_MU_IER_REG,0);		// disable interrupts
	write32(AUX_MU_CNTL_REG,0); 	// use default (RTS) flow control
	write32(AUX_MU_MCR_REG,0);		// RTS is active high (default)
	write32(AUX_MU_IIR_REG,6);		// clear the fifos
		// dwelch used 0xC6, but the doc says those extra bits
		// are readonly-0, and they don't make any difference in
		// my usage.
	write32(AUX_MU_LCR_REG,3);		// undocumented value for LCR register
		// from errata: "LCR register, bit 1 must be set for 8 bit mode,
		// like a 16550 write a 3 to get 8-bit mode" 
	write32(AUX_MU_BAUD_REG,(250000000/8 + nBaudrate/2) / nBaudrate - 1);
		// what, does one register actually work as advertised?!?!
	
	// setting the clock here (ala dwelch) does not work for me
	// it works on circle without this snipped, so ...
	
	#if 0	// doesn't work
		for(ra=0;ra<150;ra++) ra=0;	// dummy(ra);
		write32(ARM_GPIO_GPPUDCLK0,(1<<14)|(1<<15));
		for(ra=0;ra<150;ra++) ra=0;	// dummy(ra);
		write32(ARM_GPIO_GPPUDCLK0,0);
	#endif
	
	// start it up
	
	write32(AUX_MU_IER_REG,0xf);	// enable interrupts and mysterious bits
		// I try setting all four IER low order (mysteryous) bits, but
		// I'm still not getting interrupts, and on readback the register is 0x03
	write32(AUX_MU_CNTL_REG,3);		// enable receive and transmit

	// first, last?  doesn't seem to matter.
	// I'm not getting interrupts from the mini-uart :-(
	
	if (m_pInterruptSystem)
	{
		// m_pInterruptSystem->ConnectIRQ(ARM_IRQ_AUX, InterruptStub, this);
		m_pInterruptSystem->ConnectIRQ(ARM_IRQ_GPIO1, InterruptStub, this);
		m_bInterruptConnected = TRUE;
	}
	
	// finished, tell the world about it ...
		
	PeripheralExit();	
	CDeviceNameService::Get ()->AddDevice ("ttyS2", this, FALSE);
	CLogger::Get()->Write("miniuart",LogDebug,"CMiniUartDevice::Initialize() finished");
	Write("CMiniUartDevice started ...\n",28);
	if (m_bInterruptConnected)
		Write("    using interrupts\n",21);
	return true;
}


int CMiniUartDevice::Write (const void *pBuffer, size_t nCount)
{
	u8 *c = (u8 *) pBuffer;
	for (unsigned i=0; i<nCount; i++)
	{
		// uart_send(*c++);
		while(1)
		{
			if(read32(AUX_MU_LSR_REG)&0x20) break;
		}
		write32(AUX_MU_IO_REG,*c++);		
	}
	
	// uart_flush();
	while(1)
	{
		if((read32(AUX_MU_LSR_REG)&0x100)==0) break;
	}
	
	return nCount;
}



int CMiniUartDevice::Read (void *pBuffer, size_t nCount)
{
	if(!(read32(AUX_MU_LSR_REG)&0x01))
		return 0;
	u8 v = read32(AUX_MU_IO_REG) & 0xFF;
	u8 *p = (u8 *) pBuffer;
	*p = v;
	return 1;
}


void CMiniUartDevice::InterruptHandler (void)
{
}

// static
void CMiniUartDevice::InterruptStub (void *pParam)
{
	printf("got mini-uart interrupt\n");
	CMiniUartDevice *pThis = (CMiniUartDevice *) pParam;
	pThis->InterruptHandler();
}
