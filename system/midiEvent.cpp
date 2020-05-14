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
		s16 cable,
		s16 channel,
		midiEventType_t type,
		s16 value1,
		s16 value2 ) :
	midiEvent(cable,channel,0,value1,value2),
	m_pObject(pObject),
	m_pMethod(pMethod),
	m_type(type)
{
	m_pNextHandler = 0;
	m_pPrevHandler = 0;
}



// static
void midiEventHandler::registerMidiHandler(
	void *pObject,
	handleMidiEventFxn pMethod,
	s16 cable,
	s16 channel,
	midiEventType_t type,
	s16 value1,
	s16 value2)
{
	#if 0
		LOG("registerMidiHandler(0x%08X,0x%02X,0x%02X,%d,0x%02X,0x%02x)",
			(u32)pObject,
			(cable&0xff),channel&0xff,type,value1&0xff,value2&0xff);
	#endif

	midiEventHandler *handler = new midiEventHandler(pObject,pMethod,cable,channel,type,value1,value2);
	
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
	s16 cable,
	s16 channel,
	midiEventType_t type,
	s16 value1,
	s16 value2)
{
	#if 0
		LOG("unRegisterMidiHandler(0x%08X,0x%02X,0x%02X,%d0x%02X,0x%02X)",
			(u32)pObject,
			(cable&0xff),channel&0xff,type,value1&0xff,value2&0xff);
	#endif

	midiEventHandler *found = 0;
	midiEventHandler *cur = m_sFirstHandler;
	while (cur)
	{
		if (pObject == cur->m_pObject &&
			pMethod == cur->m_pMethod &&
			cable == cur->m_cable &&
			channel == cur->m_channel &&
			type == cur->m_type &&
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
void midiEventHandler::dispatchMidiEvent(midiEvent *pEvent,u8 *nrpn)
{
	#if 0
		LOG("dispatchMidiEvent(0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X)",
			(pEvent->getCable() & 0xff),
			(pEvent->getChannel() & 0xff),
			(pEvent->getMsg() & 0xff),
			(pEvent->getValue1() & 0xff),
			(pEvent->getValue2() & 0xff),
			(nrpn[0] & 0xff),
			(nrpn[1] & 0xff)); 
	#endif
	
	
	midiEventHandler *cur = m_sFirstHandler;
	while (cur)
	{
		#if 0
			LOG("checking cur(0x%02X,0x%02X,%d,0x%02X,0x%02X)",
				(cur->getCable() & 0xff),
				(cur->getChannel() & 0xff),
				cur->m_type,
				(cur->getValue1() & 0xff),
				(cur->getValue2() & 0xff)); 
		#endif
		
		if (((cur->m_cable   == -1)  || (cur->m_cable   == pEvent->getCable()   )) &&
			((cur->m_channel == -1)  || (cur->m_channel == pEvent->getChannel() )) )
		{
			bool sendit = false;
			s16 msg = pEvent->getMsg();
			s16 p1 = pEvent->getValue1();
			s16 p2 = pEvent->getValue2();
			
			if ((cur->m_type == MIDI_EVENT_TYPE_NOTE) &&
				(msg == MIDI_EVENT_NOTE_ON ||
				 msg == MIDI_EVENT_NOTE_OFF) &&
				(cur->m_value1 == -1 || cur->m_value1 == p1) &&
				(cur->m_value2 == -1 || cur->m_value2 == p2))
			{
				sendit = true;
			}
			else
			if ((cur->m_type == MIDI_EVENT_TYPE_CC) &&
				(msg == MIDI_EVENT_CC) &&
				(cur->m_value1 == -1 || cur->m_value1 == p1) &&
				(cur->m_value2 == -1 || cur->m_value2 == p2))
			{
				sendit = true;	
			}
			else
			if ((cur->m_type == MIDI_EVENT_TYPE_INC_DEC1) &&
				(msg == MIDI_EVENT_CC) &&
				(p1 == MIDI_CC_DECREMENT ||
				 p1 == MIDI_CC_INCREMENT) &&
				(cur->m_value1 == -1 || cur->m_value1 == nrpn[0]) &&
				(cur->m_value2 == -1 || cur->m_value2 == nrpn[1]))
			{
				sendit = true;
			}
			else
			if ((cur->m_type == MIDI_EVENT_TYPE_INC_DEC2) &&
				(msg == MIDI_EVENT_CC) &&
				(cur->m_value1 == -1 || cur->m_value1 == p1))
			{
				// REMAP THE MIDI EVENT TYPE and VALUE !!!
				// I could scale the value here

				pEvent->m_value1 = p2 >= 0x40 ? MIDI_CC_DECREMENT : MIDI_CC_INCREMENT;
				pEvent->m_value2 = p2 >= 0x40 ? 0x80 - p2 : p2;

				// LOG("remapping CC 0x%02x val 0x%02x to val1 0x%02x val2=0x%02x", p1, p2, pEvent->m_value1, pEvent->m_value2 );
				sendit = true;
			}
			
			if (sendit)
			{
				// LOG("    dispatching to 0x%08X::0x%08X",(u32)cur->m_pObject,(u32)cur->m_pMethod);
				(cur->m_pMethod)(cur->m_pObject,pEvent);
			}
		}
		cur = cur->m_pNextHandler;
	}
}

