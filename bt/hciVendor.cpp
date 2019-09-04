//
// hciVendor.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2017  R. Stange <rsta2@o2online.de>
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
#include "_hci_vendor_defs.h"
#include "hciVendor.h"
#include "hciLayer.h"
#include <assert.h>
#include <circle/util.h>
#include <circle/logger.h>
#include <circle/sched/scheduler.h>



static const u8 Firmware[] =
{
	// prh added BCM4345C0.h for rPi 3B+
	// added PLUS3B=1 (or 0) in Rules.mk
	
	#if PLUS3B==1
		#include "BCM4345C0.h"
	#else
		#include "BCM43430A1.h"
	#endif
};


static const char *log_name = "hciVend";


hciVendor::hciVendor(hciBase *pHCIBase) :
	m_pHCIBase(pHCIBase),
	m_is_setup(false)
{
}


void hciVendor::setup()
{
	LOG("setup()",0);
	
	#if 0
		u8 buf[HCI_COMMAND_HEADER_SIZE];					// the command has no parameters
		u8 *p = buf;
		SET_WORD(p,HCI_OP_VENDOR_DOWNLOAD_MINIDRIVER);		// opcode (no handle)
		SET_BYTE(p,0);              						// length byte == 0, no parameters
		m_pHCIBase->getHCILayer()->sendCommand(&buf, HCI_COMMAND_HEADER_SIZE);
	#else
		hci_command_header cmd;
		cmd.opcode = HCI_OP_VENDOR_DOWNLOAD_MINIDRIVER;
		m_pHCIBase->getHCILayer()->sendCommand(&cmd, sizeof cmd);
	#endif
	
	CScheduler::Get()->MsSleep(50);
	m_nFirmwareOffset = 0;
	m_is_setup = false;
}



bool hciVendor::processCommandCompleteEvent(const void *buffer, u16 length)
{
	assert(length >= sizeof(hci_event_header));
	hci_command_complete_event *pCommandComplete = (hci_command_complete_event *) buffer;
	
	switch (pCommandComplete->command_opcode)
	{
		case HCI_OP_VENDOR_DOWNLOAD_MINIDRIVER:
		case HCI_OP_VENDOR_WRITE_RAM:
		{
			// if (m_nFirmwareOffset == 0)
			// 	LOG("WRITE_RAM(0x%04x)",	// offset=%d/%d",
			// 		pCommandComplete->command_opcode,
			// 		m_nFirmwareOffset,
			// 		sizeof(Firmware));

			assert(m_nFirmwareOffset+3 <= sizeof(Firmware));
		
			u16 nopcode  = Firmware[m_nFirmwareOffset++];
				nopcode |= Firmware[m_nFirmwareOffset++] << 8;
			u8 len = Firmware[m_nFirmwareOffset++];

			hci_vendor_command cmd;
			cmd.hdr.opcode = nopcode;
			cmd.hdr.length = len;
			for (unsigned i=0; i<len; i++)
			{
				assert(m_nFirmwareOffset < sizeof Firmware);
				cmd.data[i] = Firmware[m_nFirmwareOffset++];
			}

			m_pHCIBase->getHCILayer()->sendCommand(&cmd, sizeof cmd.hdr + len);
			return true;
		}

		case HCI_OP_VENDOR_LAUNCH_RAM:
			LOG("setup complete",0);
			CScheduler::Get()->MsSleep(250);
			m_is_setup = true;
			m_pHCIBase->setup();
			return true;
	}
	
	return false;
}

