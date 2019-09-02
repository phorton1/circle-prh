//
// Circle Squared - for lack of a better name
// Copyright (c) 2019 Patrick Horton
//
// A generalized framework for creating audio-midi-ui applications
// in bare metal on the Raspberry Pi initially based ond the Circle
// rPi Bare Metal OS and the Teensy Audio Library.
//
// The Circle rPi Bare Metal OS is
// Copyright (x) 2017-2010  R. Stange, rsta2@o2online.de
//
// The Teensy Audio Library is
// Copyright (c) 2010, Paul Stoffregen, paul@pjrc.com

#ifndef _systemEvent_h
#define _systemEvent_h

#include <circle/types.h>


//------------------------------------
// systemEvent 
//------------------------------------
// System events tie everything together.
// Derived classes include
//
//     ui_events
//     audio_events
//     midi events.
//
// Special cases of other event-like behavior is
// handled in specific subsystems. Mouse and touch
// events are internal to the ui subsystem.
//
// Objects from a subsystem only respond to the events specific to
// that subsystem.
//
// Inter-system, any object can be bound to any other object so that
// an event mapping may occur.  For example, an audio device might
// receive a control event to change the volume, which it does.
// It then may be bound to a user interface control which will
// display the volume, so a control value event to that object
// is enqueued for.

#define OBJ_ID_ANY      0xffffffff


class systemEvent
{
	public:
	
		systemEvent(
				u32   event_type,
				u32   event_id,
				s32   event_value = 0,
				void *src_obj 	  = 0,
				u32   src_id 	  = OBJ_ID_ANY,
				void *dest_obj	  = 0,
				u32   dest_id 	  = OBJ_ID_ANY) :
			m_event_type(event_type),
			m_event_id(event_id),
			m_event_value(event_value),
			m_src_obj(src_obj),
			m_src_id(src_id),
			m_dest_obj(dest_obj),
			m_dest_id(dest_id)
		{
			printf("new event value=%d\n",m_event_value);
			m_pNext = 0;
			m_pPrev = 0;
		}

		~systemEvent() {}
		
		u32 getEventType() const		{ return m_event_type; }
		u32 getEventID() const			{ return m_event_id; }
		s32 getEventValue() const		{ return m_event_value; }
		void *getSrcObject() const		{ return m_src_obj; }
		u32 getSrcID() const			{ return m_src_id; }
		void *getDestObject() const		{ return m_dest_obj; }
		u32 getDestID() const			{ return m_dest_id; }
		
	protected:
		
		u32 m_event_type;
		u32 m_event_id;
		s32 m_event_value;
		void *m_src_obj;
		u32 m_src_id;
		void *m_dest_obj;
		u32 m_dest_id;
		
		systemEvent *m_pNext;
		systemEvent *m_pPrev;
		
};	// systemEvent




//------------------------------------
// systemEventHandler
//------------------------------------

#define EVENT_HANDLED   0x00000001

#define EVENT_TYPE_ALL  0xffffffff
#define EVENT_ID_ALL    0xffffffff

#define EVENT_TYPE_AUDIO_CONTROL   0x00000001

class systemEventHandler 
{
	public:
		
		systemEventHandler(
			u32 id,
			u32 event_type_mask = EVENT_TYPE_ALL,
			u32 event_id_mask = EVENT_ID_ALL) :
			m_id(id),
			m_event_type_mask(event_type_mask),
			m_event_id_mask(event_id_mask),
			m_num_listeners(0),
			m_pFirstListener(0),
			m_pLastListener(0),
			m_pNextListener(0),
			m_pPrevListener(0)	{}
		~systemEventHandler() 	{}
		
		u32 getID()				{ return m_id; }
		u32 getEventTypeMask()  { return m_event_type_mask; }
		u32 getEventIDMask()  	{ return m_event_id_mask; }
		
		void addEventListener(systemEventHandler *listener)
		{
			if (m_pLastListener)
			{
				m_pLastListener->m_pNextListener = listener;
				listener->m_pPrevListener = m_pLastListener;
			}
			else
			{
				m_pFirstListener = listener;
			}
			m_pLastListener = listener;
		}
		
		void removeEventListener(systemEventHandler *listener)
		{
			if (listener->m_pPrevListener)
				listener->m_pPrevListener->m_pNextListener = listener->m_pNextListener;
			if (listener->m_pNextListener)
				listener->m_pNextListener->m_pPrevListener = listener->m_pPrevListener;
			if (listener == m_pFirstListener)
				m_pFirstListener = listener->m_pNextListener;
			if (listener == m_pFirstListener)
				m_pLastListener = listener->m_pPrevListener;
		}
		
		u16 getNumListeners() const { return m_num_listeners; }
		systemEventHandler *getFirstListener() const { return m_pFirstListener; }
		systemEventHandler *getLastListener() const { return m_pLastListener; }
		systemEventHandler *getNextListener() const { return m_pNextListener; }
		systemEventHandler *getPrevListener() const { return m_pPrevListener; }
		
		virtual u32 handleEvent(systemEvent *event)
		{
			for (systemEventHandler *p=m_pFirstListener; p; p=p->m_pNextListener)
			{
				if ( (event->getEventType() & p->getEventTypeMask())  &&
					 (event->getEventID() & p->getEventIDMask()) )
				{
					u32 rslt = p->handleEvent(event);
					if (rslt == EVENT_HANDLED)
					{
						delete event;
						return rslt;
					}
				}
			}
			delete event;
			return 0;
		}
		
	
	protected:
		
		u32 m_id;
		
		u32 m_event_type_mask;
		u32 m_event_id_mask;     
		
		u16					m_num_listeners;
		systemEventHandler *m_pFirstListener;	// list of regisgtered listeners
		systemEventHandler *m_pLastListener;
		systemEventHandler *m_pNextListener;	// next and prev within that list
		systemEventHandler *m_pPrevListener;
		
};	// systemEventHandler


#endif  // !_systemEvent_h
