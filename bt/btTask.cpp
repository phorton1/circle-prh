//
// bttask.cpp
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
#include "btTask.h"
#include <circle/sched/scheduler.h>
#include <assert.h>


btTask::btTask(hciLayer *pHCI)
	:	m_pHCI(pHCI)
{
	assert(pHCI != 0);
}


btTask::~btTask(void)
{
	m_pHCI = 0;
}


void btTask::Run(void)
{
	while (1)
	{	
		m_pHCI->process();
		CScheduler::Get()->Yield();
	}
}
