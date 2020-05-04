//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsTopWindow.h"
#include "wsApp.h"
#include "wsEvent.h"

#include <circle/logger.h>
#define log_name  "wstop"


//----------------------------------------------
// wsTopLevelWindow
//----------------------------------------------

wsTopLevelWindow::wsTopLevelWindow(wsApplication *pParent, u16 id, s32 xs, s32 ys, s32 xe, s32 ye, u32 style) :
	wsWindow(pParent,id,xs,ys,xe,ye,style | WIN_STYLE_TOP_LEVEL)
{
	m_pPrevWindow = 0;
	m_pNextWindow = 0;
	pParent->addTopLevelWindow(this);
}


wsWindow* wsTopLevelWindow::hitTest(s32 x, s32 y)
	// the current top level window is presumed
	// to be visible ... 
{
	wsWindow *found = 0;
	wsWindow *p = m_pFirstChild;
	while (p)
	{
		found = p->hitTest(x,y);
		if (found)
			p = 0;
		else
			p = p->m_pNextSibling;
	}
	// LOG("hitTest() found=%08x",found);
	if (found)
	{
		// printf("wsTopLevelWindow::setTouchFocus(%08x)\n",found);
		getApplication()->setTouchFocus(found);
	}
	
	// if the click is outside of the object, and it is registered,
	// directly generate a WIN_EVENT_CLICK_OUTSIDE event, which
	// the window will then use to hide itself.
	
	else if (m_style & WIN_STYLE_POPUP)
	{
		LOG("%08x:%d WIN_STYLE_POPUP generating WIN_EVENT_CLICK_OUTSIDE",(u32)this,m_id);
		if (!m_rect_abs.intersects(x,y))
		{
			getApplication()->addEvent(new wsEvent(
				EVT_TYPE_WINDOW,
				WIN_EVENT_CLICK_OUTSIDE,
				this ));
		}
	}
	
	
	return found;
}


u32 wsTopLevelWindow::handleEvent(wsEvent *event)
{
	#if 0
	// WS_FRAME_EVENT messes up this debugging
	LOG("%08x:%d handleEvent(%08x,%04x)",(u32)this,m_id,event->getEventType(),event->getEventID());
	#endif
	
	u32 result_handled = 0;
	if (event->getEventType() == EVT_TYPE_WINDOW &&
		event->getEventID() == WIN_EVENT_CLICK_OUTSIDE)
	{
		LOG("%08x:%d WIN_EVENT_CLICK_OUTSIDE hiding self",(u32)this,m_id);
		assert(event->getObject() == this);
		debugUpdate(1);
		
		// not that we don't try to dispatch an event 
		// from an event handler
		//
		// you cannot call addEvent() from handleEvent() !!!
		// and addPendingEvent() is not implemented yet!
		
		hide();
		result_handled = 1;
	}
	if (!result_handled)
		result_handled = wsWindow::handleEvent(event);
	
	return result_handled; 	// return value unclear
};


// In addtion to base class behavior, these methods add or
// remove the top level window from the app's list of top level
// windows.
//
// This should probably be generalized to the base object
// with a zorder for things like bringing the currently
// dragging object (temporarily) to the top of the window's
// list of objects.

void wsTopLevelWindow::show()
{
	if (!(m_state & WIN_STATE_VISIBLE))
	{
		wsWindow::show();
		getApplication()->addTopLevelWindow(this);
	}
}

void wsTopLevelWindow::hide()
{
	if (m_state & WIN_STATE_VISIBLE)
	{
		wsWindow::hide();
		getApplication()->removeTopLevelWindow(this);
	}
}

