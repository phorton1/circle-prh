#include "midiEvent.h"
#include <utils/myUtils.h>
#include <circle/logger.h>
#define log_name  "midi"


// static
midiEventHandler *midiEventHandler::m_sFirstHandler = 0;
midiEventHandler *midiEventHandler::m_sLastHandler = 0;


// protected
midiEventHandler::midiEventHandler(
		void *pObject,
		handleMidiEventFxn pMethod,
		s8 cable,
		s8 channel,
		s8 msg,
		s8 value1,
		s8 value2 ) :
	midiEvent(0,cable,channel,msg,value1,value2),
	m_pObject(pObject),
	m_pMethod(pMethod)
{
	m_pNextHandler = 0;
	m_pPrevHandler = 0;
}



// static
void midiEventHandler::registerMidiHandler(
	void *pObject,
	handleMidiEventFxn pMethod,
	s8 cable,
	s8 channel,
	s8 msg,
	s8 value1,
	s8 value2)
{
	// LOG("registerMidiHandler(0x%08X,0x%08X,%d,%d,%d,%d,%d)",(u32)pObject,(u32)pMethod,cable,channel,msg,value1,value2);

	midiEventHandler *handler = new midiEventHandler(pObject,pMethod,cable,channel,msg,value1,value2);
	
	if (m_sLastHandler)
	{
		handler->m_pPrevHandler = m_sLastHandler;
		m_sLastHandler->m_pNextHandler = handler;
	}
	else
	{
		m_sFirstHandler = handler;
	}
	m_sLastHandler = handler;
	
	int count = 0;
	midiEventHandler *cur = m_sFirstHandler;
	while (cur)
	{
		count++;
		cur = cur->m_pNextHandler;
	}
	// LOG("    numHandlers=%d",count);
}


void midiEventHandler::unRegisterMidiHandler(
	void *pObject,
	handleMidiEventFxn pMethod,
	s8 cable,
	s8 channel,
	s8 msg,
	s8 value1,
	s8 value2)
{
	// LOG("unRegisterMidiHandler(0x%08X,0x%08X,%d,%d,%d,%d,$d)",(u32)pObject,(u32)pMethod,cable,channel,msg,value1,value2);

	midiEventHandler *found = 0;
	midiEventHandler *cur = m_sFirstHandler;
	while (cur)
	{
		if (pObject == cur->m_pObject &&
			pMethod == cur->m_pMethod &&
			cable == cur->m_cable &&
			channel == cur->m_channel &&
			msg == cur->m_msg &&
			value1 == cur->m_value1 &&
			value2 == cur->m_value2)
		{
			found = cur;
			break;
		}
		cur = cur->m_pNextHandler;
	}
	if (found)
	{
		LOG("    deleting midi handler",0);
		if (found == m_sLastHandler)
		{
			m_sLastHandler = found->m_pPrevHandler;
		}
		if (found == m_sFirstHandler)
		{
			m_sFirstHandler = found->m_pNextHandler;
		}
		if (found->m_pNextHandler)
		{
			found->m_pNextHandler->m_pPrevHandler = found->m_pPrevHandler;
		}
		if (found->m_pPrevHandler)
		{
			found->m_pPrevHandler->m_pNextHandler = found->m_pNextHandler;
		}
		delete found;
	}
	else
	{
		LOG_WARNING("   could not find midi handler to unRegister",0);
	}
}


// static
void midiEventHandler::dispatchMidiEvent(midiEvent *pEvent)
{
	LOG("dispatchMidiEvent(%d,%d,%d,%d,$d)",
		pEvent->getCable(),
	    pEvent->getChannel(),
	    pEvent->getMsg(),    
	    pEvent->getValue1(),
	    pEvent->getValue2()); 
	
	midiEventHandler *cur = m_sFirstHandler;
	while (cur)
	{
		if (
			(cur->m_cable   == -1  || cur->m_cable   == pEvent->getCable()   ) &&
			(cur->m_channel == -1  || cur->m_channel == pEvent->getChannel() ) &&
			(cur->m_msg     == -1  || cur->m_msg     == pEvent->getMsg()     ) &&
			(cur->m_value1  == -1  || cur->m_value1  == pEvent->getValue1()  ) &&
			(cur->m_value2  == -1  || cur->m_value2  == pEvent->getValue2()  ) )
		{
			LOG("    dispatching to 0x%08X::0x%08X",(u32)cur->m_pObject,(u32)cur->m_pMethod);
			(cur->m_pMethod)(cur->m_pObject,pEvent);
		}
		cur = cur->m_pNextHandler;
	}
}

