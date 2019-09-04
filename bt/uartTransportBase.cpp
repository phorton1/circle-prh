//
// tranportUart.cpp
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

#include "_hci_defs.h"
#include "uartTransportBase.h"
#include <circle/memio.h>
#include <circle/logger.h>
#include <assert.h>


// prh temporary static global for debugging

int show_transport_bytes = 0;


enum enumUartState
{
	RxStateStart,
	RxStateDataHeader,		
	RxStateEventHeader,
	RxStateEventLength,
	RxStateContent,
	RxStateUnknown
};



static const char log_name[] = "bttpbase";

// The GPIO 14 & 15 pins should not be touched if the MINI_UART
// has already been initialized, yet otherwise, for some reason,
// it seems necessary to set them or the transport does not work,
// even without a conflicting CSerial device.  I'm not completely
// clear on why this is necessary, and still have concerns that
// it is order-of-operation dependent.

uartTransportBase::uartTransportBase()
:
	m_pPacketHandler(0),
	m_nRxState(RxStateStart)
{
}


uartTransportBase::~uartTransportBase(void)
{
	m_pPacketHandler = 0;
}


void uartTransportBase::registerPacketHandler(void *pHCILayer, TBTHCIEventHandler *pHandler)
{
	assert(m_pPacketHandler == 0);
	m_pHCI = pHCILayer;
	m_pPacketHandler = pHandler;
	assert(m_pPacketHandler != 0);
}



//---------------------------------
// output
//---------------------------------

boolean uartTransportBase::SendHCICommand(const void *pBuffer, unsigned length)
{
	u8 *pChar = (u8 *) pBuffer;
	assert(pChar != 0);

	PeripheralEntry();
	Write(HCI_PREFIX_CMD);
	while (length--)
	{
		Write(*pChar++);
	}
	PeripheralExit();
	return TRUE;
}



boolean uartTransportBase::SendHCIData(const void *pBuffer, unsigned length)
{
	u8 *pChar = (u8 *) pBuffer;
	assert(pChar != 0);

	PeripheralEntry();
	Write(HCI_PREFIX_DATA);
	while (length--)
	{
		Write(*pChar++);
	}
	PeripheralExit();
	return TRUE;
}





//---------------------------------
// input
//---------------------------------


void uartTransportBase::Receive(u8 nChar)
{
	if (show_transport_bytes)
		printf("(%02x)",nChar);

	switch (m_nRxState)
	{
		case RxStateStart:
			m_prefix_byte = nChar;
			m_nRxInPtr = 0;
			m_nRxParamLength = 0;
			if (m_prefix_byte == HCI_PREFIX_EVENT)
				m_nRxState = RxStateEventHeader;
			else if (m_prefix_byte == HCI_PREFIX_DATA)
				m_nRxState = RxStateDataHeader;
			break;

		case RxStateDataHeader:
			m_RxBuffer[m_nRxInPtr++] = nChar;
			
			// copy and skip the ACL data packet header which consists
			// of a word hci_handle|flags, and a word length .. once
			// we have copied the length, we can use it ...
			
			if (m_nRxInPtr == 4)
			{
				m_nRxParamLength = *(u16 *) &m_RxBuffer[2];
				// now using a buffer of size UART_TRANSPORT_BUFFER_SIZE
				// that can hold a full 64K of data ...
				
				// assert(m_nRxParamLength <= MAX_SIZE_BYTE);

					
				if (m_nRxParamLength == 0)
				{
					if (m_pPacketHandler != 0)
					{
						(*m_pPacketHandler)(m_pHCI, m_prefix_byte, m_RxBuffer, m_nRxInPtr);
					}
					m_nRxState = RxStateStart;
				}
				else
					m_nRxState = RxStateContent;
			}
			break;

		case RxStateEventHeader:
			// better called RxStateEventType, as this copies and
			// skips the event "type" byte ...
			m_RxBuffer[m_nRxInPtr++] = nChar;
			m_nRxState = RxStateEventLength;
			break;

		case RxStateEventLength:
			m_RxBuffer[m_nRxInPtr++] = nChar;
			if (nChar > 0)
			{
				m_nRxParamLength = nChar;
				m_nRxState = RxStateContent;
			}
			else
			{
				if (m_pPacketHandler != 0)
				{
					(*m_pPacketHandler)(m_pHCI, m_prefix_byte, m_RxBuffer, m_nRxInPtr);
				}

				m_nRxState = RxStateStart;
			}
			break;

		
		case RxStateContent:
			// this copies the bulk of the packet after the header
			assert(m_nRxInPtr < UART_TRANSPORT_BUFFER_SIZE);
				// HCI_MAX_PACKET_SIZE);
			m_RxBuffer[m_nRxInPtr++] = nChar;
			if (--m_nRxParamLength == 0)
			{
				if (m_pPacketHandler != 0)
				{
					(*m_pPacketHandler)(m_pHCI, m_prefix_byte, m_RxBuffer, m_nRxInPtr);
				}

				m_nRxState = RxStateStart;
			}
			break;

		default:
			assert(0);
			break;
	}
}


