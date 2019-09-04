//
// hciBase.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _hci_base_h_
#define _hci_base_h_

#include "_hci_defs.h"
#include "btQueue.h"
#include <circle/types.h>



extern const char *getBTErrorString(u8 status);
extern const char *getBTEventName(u8 event_code);

class hciLayer;


class hciBase
{
public:

	hciBase(hciLayer *pHCILayer, u32 device_class);
	~hciBase(void);
	
	void setLocalName(const char *local_name);
	
	void reset();
	void setup();
	bool isSetup()  	  	{ return m_is_setup; }
	
	bool processCommandCompleteEvent(const void *buffer, u16 length);

	const u8 *getLocalAddr() const	{ return m_local_addr; }
	hciLayer *getHCILayer()  const  { return m_pHCI; }
	
private:

	hciLayer *m_pHCI;
	
	bool m_is_setup;
	u32	 m_device_class;
	u8	 m_local_name[BT_NAME_SIZE];
	u8 	 m_local_addr[BT_ADDR_SIZE];
	
};


#endif
