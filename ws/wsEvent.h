//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsEvent_h
#define _wsEvent_h

#include "wsDefines.h"


//------------------------------------
// wsEvent 
//------------------------------------

#define EVT_TYPE_WINDOW    0x00000001
#define EVT_TYPE_BUTTON    0x00000002
#define EVT_TYPE_CHECKBOX  0x00000004

#define WIN_EVENT_CLICK     	0x00000001
#define WIN_EVENT_LONG_CLICK    0x00000002
#define WIN_EVENT_CLICK_OUTSIDE 0x00000010
#define WIN_EVENT_FRAME         0x00000100
	// possibly temporary - event sent to current top level
	// window on every timeSlice()

#define BTN_EVENT_PRESSED    	 0x00000001
#define CHB_EVENT_VALUE_CHANGED  0x00000001


class wsEvent
{
	public:
	
		wsEvent(u32 type, u32 id, wsWindow *obj) :
			m_type(type),
			m_id(id),
			m_obj(obj)
		{
			m_pNext = 0;
			m_pPrev = 0;
		}

		~wsEvent() {}
		
		u32 getEventType() const		{ return m_type; }
		u32 getEventID() const			{ return m_id; }
		u16 getID() const 			    { return m_obj->getID(); }
		wsWindow *getObject() const		{ return m_obj; }

	private:
		friend class wsApplication;
		
		u32 m_type;
		u32 m_id;
		wsWindow *m_obj;
		
		wsEvent *m_pNext;
		wsEvent *m_pPrev;
		
};	// wsEvent



#endif  // !_wsEvent_h
