//
// btqueue.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#include "btqueue.h"
#include "_hci_defs.h"
#include <circle/util.h>
#include <assert.h>


btBuffer::btBuffer(unsigned size)
{
	length = size;
	data = new unsigned char[length+4];
	buffer = &data[ 4 - (((u32)data) & 0x3) ];	// align
	assert(buffer);
}

btBuffer::~btBuffer()
{
	delete data;
}




btQueue::btQueue (void)
:	m_pFirst (0),
	m_pLast (0)
{
}

btQueue::~btQueue (void)
{
	flush ();
}


void btQueue::flush (void)
{
	while (m_pFirst != 0)
	{
		m_SpinLock.Acquire ();

		volatile btBuffer *pEntry = m_pFirst;
		assert (pEntry != 0);

		m_pFirst = pEntry->pNext;
		if (m_pFirst != 0)
		{
			m_pFirst->pPrev = 0;
		}
		else
		{
			assert (m_pLast == pEntry);
			m_pLast = 0;
		}

		m_SpinLock.Release ();

		delete pEntry;
	}
}



	
void btQueue::enqueue (const void *pBuffer, unsigned length)
{
	assert (length > 0);
	// assert (length <= HCI_MAX_PACKET_SIZE);
	assert (pBuffer != 0);

	btBuffer *pEntry = new btBuffer(length);
	assert (pEntry != 0);

	memcpy (pEntry->buffer, pBuffer, length);
	enqueue(pEntry);
}


void btQueue::enqueue(btBuffer *pEntry)
{
	assert(pEntry);
	
	m_SpinLock.Acquire ();

	pEntry->pPrev = m_pLast;
	pEntry->pNext = 0;

	if (m_pFirst == 0)
	{
		m_pFirst = pEntry;
	}
	else
	{
		assert (m_pLast != 0);
		assert (m_pLast->pNext == 0);
		m_pLast->pNext = pEntry;
	}
	m_pLast = pEntry;

	m_SpinLock.Release ();
}



btBuffer *btQueue::dequeue()
{
	btBuffer *nResult = 0;
	
	if (m_pFirst != 0)
	{
		m_SpinLock.Acquire ();

		volatile btBuffer *pEntry = m_pFirst;
		assert (pEntry != 0);

		m_pFirst = pEntry->pNext;
		if (m_pFirst != 0)
		{
			m_pFirst->pPrev = 0;
		}
		else
		{
			assert (m_pLast == pEntry);
			m_pLast = 0;
		}

		m_SpinLock.Release ();

		nResult = (btBuffer *) pEntry;
	}

	return nResult;
}

