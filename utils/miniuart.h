//
// miniuart.h
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
#ifndef _circle_miniuart_h
#define _circle_miniuart_h

#include <circle/device.h>
#include <circle/interrupt.h>
#include <circle/spinlock.h>


#define MINI_SERIAL_BUF_SIZE  2048



class CMiniUartDevice : public CDevice
{
public:

	CMiniUartDevice(CInterruptSystem *pInterruptSystem);
	~CMiniUartDevice (void);

	boolean Initialize (unsigned nBaudrate = 115200);
	int Write (const void *pBuffer, size_t nCount);
	int Read (void *pBuffer, size_t nCount);

private:
    
    void InterruptHandler (void);
    static void InterruptStub (void *pParam);
   
    CInterruptSystem *m_pInterruptSystem;
    boolean m_bInterruptConnected;
    
    u8 m_RxBuffer[MINI_SERIAL_BUF_SIZE];
    volatile unsigned m_nRxInPtr;
    volatile unsigned m_nRxOutPtr;
    volatile int m_nRxStatus;
    
    CSpinLock m_SpinLock;

};  // class CMiniUartDevice

#endif  // !defined(_circle_miniuart_h)

