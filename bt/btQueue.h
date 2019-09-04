//
// btqueue.h
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
#ifndef _circle_bt_btqueue_h
#define _circle_bt_btqueue_h

#include <circle/spinlock.h>
#include <circle/types.h>

class btBuffer
{
public:
    
    btBuffer(unsigned size);
    ~btBuffer();
    
	unsigned char   *buffer;
	unsigned	    length;

private:
    
    friend class btQueue;
    unsigned char       *data;
    
	volatile btBuffer	*pPrev;
	volatile btBuffer	*pNext;
};


class btQueue
{
public:
	btQueue (void);
	~btQueue (void);

    void enqueue (const void *pBuffer, unsigned length);
        // create a queueEntry, fill it in, and enqueue it
    void enqueue(btBuffer *pEntry);
        // adds the entry to the linked list
    btBuffer *dequeue();
        // you must delete the btBuffer
        // when you are done with it
    bool avail()  { return m_pFirst != 0; }

private:

	void flush (void);

	volatile btBuffer *m_pFirst;
	volatile btBuffer *m_pLast;

	CSpinLock m_SpinLock;
};

#endif
