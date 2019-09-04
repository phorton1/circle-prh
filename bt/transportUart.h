//
// transportUart.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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

#ifndef _transport_uart_h
#define _transport_uart_h

#include "uartTransportBase.h"
#include <circle/interrupt.h>
#include <circle/gpiopin.h>


class transportUart : public uartTransportBase
{
public:
    
	transportUart(CInterruptSystem *pInterruptSystem);
	~transportUart(void);

	virtual void initialize(unsigned nBaudrate = 115200);


private:

	virtual void Write(u8 nChar);

	void IRQHandler(void);
	static void IRQStub(void *pParam);
    
    CGPIOPin m_GPIO14;
    CGPIOPin m_GPIO15;
	CGPIOPin m_TxDPin;
	CGPIOPin m_RxDPin;

	CInterruptSystem *m_pInterruptSystem;
	boolean m_bIRQConnected;

};


#endif
