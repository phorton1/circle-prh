// hciVendor.h
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
#ifndef _hci_vendor_h_
#define _hci_vendor_h_

#include "_hci_defs.h"
#include "btQueue.h"
#include "hciBase.h"
#include <circle/types.h>


// I don't think this "state" machine is necessary
// all it does is assert the behaviors of the events.


class hciVendor
{
public:
	
	hciVendor(hciBase *pHCIBase);
	~hciVendor(void) {}

	void setup();
	bool processCommandCompleteEvent(const void *buffer, u16 length);

	boolean isSetup(void) const
		{ return m_is_setup; }
	
private:
	
	hciBase *m_pHCIBase;
	bool m_is_setup;
	unsigned m_nFirmwareOffset;
	
};

#endif
