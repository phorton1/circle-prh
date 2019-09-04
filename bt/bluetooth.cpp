//
// bluetooth.cpp
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
//
#include "bluetooth.h"
#include "btTask.h"
#include <circle/logger.h>
#include <circle/sched/scheduler.h>
#include <assert.h>


static const char *log_name = "bt";



bluetoothAdapter::bluetoothAdapter()
:	m_HCI(),
	m_LCAP(&m_HCI),
	m_SDP(&m_LCAP),
	m_RFCOMM(&m_LCAP,&m_SDP)
{
}


bluetoothAdapter::~bluetoothAdapter(void)
{
}


boolean bluetoothAdapter::Initialize(btTransportBase *pTransport
		#if HCI_USE_FAT_DATA_FILE
			,FATFS *pFileSystem
		#endif
	)
{
	LOG("Bluetooth starting ...",0);
	
    m_LCAP.registerClient(&m_SDP);
    m_LCAP.registerClient(&m_RFCOMM);
	
	if (!m_HCI.initialize(pTransport
		#if HCI_USE_FAT_DATA_FILE
			,pFileSystem
		#endif
		)) return FALSE;

	new btTask(&m_HCI);

	while (!m_HCI.isSetup())
	{
		CScheduler::Get()->MsSleep(10);
		CScheduler::Get()->Yield();
	}

	LOG("Bluetooth running",0);
	return TRUE;
}

