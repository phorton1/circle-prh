//
// tftpbootserver.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2019  R. Stange <rsta2@o2online.de>
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
#include "tftpbootserver.h"
#include <circle/chainboot.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

static const char FromBootServer[] = "tftpboot";

CTFTPBootServer::CTFTPBootServer (CNetSubSystem *pNetSubSystem, size_t nMaxKernelSize)
:	CTFTPDaemon (pNetSubSystem),
	m_nMaxKernelSize (nMaxKernelSize),
	m_bFileOpen (FALSE),
	m_pKernelBuffer (0)
{
	client_connected = 0;
}

CTFTPBootServer::~CTFTPBootServer (void)
{
	assert (!m_bFileOpen);
	delete [] m_pKernelBuffer;
	m_pKernelBuffer = 0;
}

boolean CTFTPBootServer::FileOpen (const char *pFileName)
{
	return FALSE;
}

boolean CTFTPBootServer::FileCreate (const char *pFileName)
{
	client_connected = true;
	if (m_bFileOpen)
	{
		return FALSE;
	}

	assert (pFileName != 0);
	static const char FileBaseName[] = "kernel";
	if (strncmp (pFileName, FileBaseName, sizeof FileBaseName-1) != 0)
	{
		return FALSE;
	}

	static const char FileExt[] = ".img";
	size_t nLen = strlen (pFileName);
	assert (nLen > sizeof FileExt);
	if (strcmp (&pFileName[nLen - (sizeof FileExt-1)], FileExt) != 0)
	{
		return FALSE;
	}

	CLogger::Get ()->Write (FromBootServer, LogDebug, "Receiving %s ...", pFileName);
	if (m_pKernelBuffer == 0)
	{
		m_pKernelBuffer = new u8[m_nMaxKernelSize];
		if (m_pKernelBuffer == 0)
		{
			return FALSE;
		}
	}

	m_nCurrentOffset = 0;
	m_bFileOpen = TRUE;
	return TRUE;
}


boolean CTFTPBootServer::FileClose (boolean bOK)
	// Circle base class DID NOT pass success boolean,
	// and DOES NOT check return value, sheesh.
{
	assert (m_bFileOpen);
	m_bFileOpen = FALSE;
	if (!bOK)
	{
		CLogger::Get ()->Write(FromBootServer, LogError, "Error in TFTP receive after %lu bytes", m_nCurrentOffset);
		delete m_pKernelBuffer;
		m_pKernelBuffer = 0;
		m_nCurrentOffset = 0;
		client_connected = false;
	}
	else
	{
		CLogger::Get ()->Write(FromBootServer, LogDebug, "succesfully received %lu bytes", m_nCurrentOffset);
		if (m_nCurrentOffset > 0)
		{
			CLogger::Get ()->Write (FromBootServer, LogNotice, "enabling chain boot for TFTP file");
			EnableChainBoot (m_pKernelBuffer, m_nCurrentOffset);
		}
	}
	return TRUE;
}


int CTFTPBootServer::FileRead (void *pBuffer, unsigned nCount)
{
	return -1;
}


int CTFTPBootServer::FileWrite (const void *pBuffer, unsigned nCount)
{
	assert (m_bFileOpen);
	if (m_nCurrentOffset + nCount > m_nMaxKernelSize)
	{
		m_bFileOpen = FALSE;
		return -1;
	}
	assert (pBuffer != 0);
	memcpy (m_pKernelBuffer + m_nCurrentOffset, pBuffer, nCount);
	m_nCurrentOffset += nCount;
	return nCount;
}
