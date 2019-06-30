
#include "AudioConnection.h"
#include "AudioStream.h"


void AudioConnection::connect(void)
{
	if (m_bConnected)
		return;
	if (m_destIndex > m_dest.m_numInputs)
		return;
	
	__disable_irq();
	
	AudioConnection *p = m_src.m_pFirstConnection;
	if (p == NULL)
	{
		m_src.m_pFirstConnection = this;
	}
	else
	{
		while (p->m_pNextConnection)
		{
			if (&p->m_src == &this->m_src &&
				&p->m_dest == &this->m_dest &&
				p->m_srcIndex == this->m_srcIndex &&
				p->m_destIndex == this->m_destIndex)
			{
				//Source and destination already connected through another connection, abort
				__enable_irq();
				return;
			}
			p = p->m_pNextConnection;
		}
		p->m_pNextConnection = this;
	}
	
	this->m_pNextConnection = NULL;

	m_src.m_numConnections++;
	m_dest.m_numConnections++;

	m_bConnected = true;

	__enable_irq();
}


void AudioConnection::disconnect(void)
{
	if (!m_bConnected)
		return;
	if (m_destIndex > m_dest.m_numInputs)
		return;
	
	__disable_irq();
	
	// Remove destination from source list
	
	AudioConnection *p = m_src.m_pFirstConnection;
	if (p == NULL)
	{
		return;
	}
	else if (p == this)
	{
		if (p->m_pNextConnection)
		{
			m_src.m_pFirstConnection = m_pNextConnection;
		}
		else
		{
			m_src.m_pFirstConnection = NULL;
		}
	}
	else
	{
		while (p)
		{
			if (p == this)
			{
				if (p->m_pNextConnection)
				{
					p = m_pNextConnection;
					break;
				}
				else
				{
					p = NULL;
					break;
				}
			}
			p = p->m_pNextConnection;
		}
	}
	
	//Remove possible pending src block from destination
	
	m_dest.m_inputQueue[m_destIndex] = NULL;

	//Check if the disconnected AudioStream objects should still be active
	
	m_src.m_numConnections--;
	m_dest.m_numConnections--;

	m_bConnected = false;

	__enable_irq();
}


