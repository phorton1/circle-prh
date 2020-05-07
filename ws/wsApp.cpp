//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsApp.h"
#include "wsEvent.h"

#include <circle/logger.h>
#include <circle/util.h>
#define log_name  "wsapp"


//----------------------------------------------
// wsApplication
//----------------------------------------------
// all direct children of the application should be top level windows.
// events are dispatched to top level windows, which then manage their
// children.

wsApplication::~wsApplication()
{
	m_pTopWindow = 0;
	m_pBottomWindow = 0;
	m_pScreen = 0;
	m_pTouch  = 0;
	m_pMouse  = 0;
}

wsApplication::wsApplication() :
	wsWindow(0,0, 0,0,10,10,
		WIN_STYLE_APPLICATION |
		WIN_STYLE_TRANSPARENT),
	m_pScreen(0),
    m_pTouch(0),
    m_pMouse(0)
{
	LOG("ctor",0);
	m_pTopWindow = 0;
	m_pBottomWindow = 0;

	m_pTouchFocus = 0;
	memset(&m_touch_state,0,sizeof(touchState_t));
	m_lastTouchUpdate = 0;

	m_pFirstEvent = 0;
	m_pLastEvent = 0;
	m_update_frame_time = 0;
	
	m_state |= WIN_STATE_PARENT_VISIBLE;
		// the application is always progenator of
		// win_state_parent_visible
}


void wsApplication::addEvent(wsEvent *event)
{
	if (m_pLastEvent)
		event->m_pNext = m_pLastEvent;
	else
		m_pFirstEvent = event;
	m_pLastEvent = event;
}

	
void wsApplication::addTopLevelWindow(wsTopLevelWindow *pWindow)
{
	// remove it from the list if it already exists
	// so it will become the new top window
	
	removeTopLevelWindow(pWindow);
	
	if (m_pTopWindow)
	{
		pWindow->m_pPrevWindow = m_pTopWindow;
		m_pTopWindow->m_pNextWindow = pWindow;
	}
	else
		m_pBottomWindow = pWindow;
	m_pTopWindow = pWindow;
}


void wsApplication::removeTopLevelWindow(wsTopLevelWindow *pWindow)
{
	if (pWindow->m_pNextWindow)
		pWindow->m_pNextWindow->m_pPrevWindow = pWindow->m_pPrevWindow;
	if (pWindow->m_pPrevWindow)
		pWindow->m_pPrevWindow->m_pNextWindow = pWindow->m_pNextWindow;
	if (pWindow == m_pTopWindow)
		m_pTopWindow = pWindow->m_pPrevWindow;
	if (pWindow == m_pBottomWindow)
		m_pBottomWindow = pWindow->m_pNextWindow;
}


void wsApplication::Initialize(
		CScreenDeviceBase *pScreen,
		CTouchScreenBase  *pTouch,
		CMouseDevice      *pMouse)
{
	LOG("Initialize(%08x,%08x,%08x)",(u32)pScreen,(u32)pTouch,(u32)pMouse);
	assert(pScreen);
	m_pScreen = pScreen;
	m_pTouch = pTouch;
	m_pMouse = pMouse;

	// the app's base class update() method is never called
	// and it is assumed to never move and occupy the full screen.
	// We set it's size here, and explicitly call onSize() to set
	// full abs, client, and clipping rectangles.
	
	m_rect.assign(0,0,m_pScreen->GetWidth()-1, pScreen->GetHeight()-1);
	onSize();
	
	m_pDC = new wsDC(m_pScreen);
	m_pDC->setFont(wsFont8x14);
	
	// print_rect("initial invalid",&m_pDC->getInvalid());
	
	setFont(wsFont8x14);
	m_pScreen->InitializeUI(m_pDC,wsDC::driverRegisterStub);
	
	if (m_pTouch)
		m_pTouch->RegisterEventHandler(touchEventStub,this);
	if (m_pMouse)
	{
		m_pMouse->Setup(getWidth(),getHeight());
		m_pMouse->ShowCursor(TRUE);		
		m_pMouse->RegisterEventHandler(mouseEventStub,this);
	}
	
	// Call the client's Create() method
	
	Create();
	// print_rect("after create invalid",&m_pDC->getInvalid());
}


//--------------------------------------------------
// generic touch event handlers
//--------------------------------------------------

void wsApplication::mouseEventStub(
	void *pThis,
	TMouseEvent event,
	unsigned buttons,
	unsigned x,
	unsigned y)
{
	assert(pThis);
	((wsApplication *)pThis)->mouseEventHandler(event,buttons,x,y);
}

void wsApplication::mouseEventHandler(
	TMouseEvent event,
	unsigned buttons,
	unsigned x,
	unsigned y)
{
	#if 0
		LOG("mouseEventHandler(%d,%d,%s,%02x)",
			x,y,
			(event == MouseEventMouseDown) ? "down" :
			(event == MouseEventMouseMove) ? "move" :
			(event == MouseEventMouseUp)   ? "up" : "unknown",
			buttons);
	#endif		

	u8 touch_event =
		(event == MouseEventMouseMove) ?
			buttons ? TOUCH_MOVE : 0 :
		(event == MouseEventMouseDown) ?
			TOUCH_DOWN :
		(event == MouseEventMouseUp) ?
			TOUCH_UP : 0;
	
	if (touch_event)
		onTouchEvent(x,y,touch_event);
}



void wsApplication::touchEventStub(
	void *pThis,
	TTouchScreenEvent event,
	unsigned id,
	unsigned x,
	unsigned y)
{
	assert(pThis);
	((wsApplication *)pThis)->touchEventHandler(event,id,x,y);
}

void wsApplication::touchEventHandler(
	TTouchScreenEvent event,
	unsigned id,
	unsigned x,
	unsigned y)
{
	#if 0
		LOG("touchEventHandler(%d,%d,%s)",
			x,y,
			(event == TouchScreenEventFingerDown) ? "down" :
			(event == TouchScreenEventFingerMove) ? "move" :
			(event == TouchScreenEventFingerUp)   ? "up" : "unknown");
	#endif		

	// LOG("touch event(%d,%d,%d,%d)",event,id,x,y);
	u8 touch_event =
		(event == TouchScreenEventFingerDown) ? TOUCH_DOWN :
		(event == TouchScreenEventFingerMove) ? TOUCH_MOVE :
		(event == TouchScreenEventFingerUp)   ? TOUCH_UP : 0;
	if (touch_event)
		onTouchEvent(x,y,touch_event);
}

	
void wsApplication::setTouchFocus(wsWindow *win)
	// called from hitTest on the the wsWindow object,
	// if any, that matched the starting x,y coordinates
{
	m_pTouchFocus = win;
	#if DEBUG_TOUCH
		LOG("setTouchFocus(%08x)",(u32)win);
	#endif		
	if (!win)
		memset(&m_touch_state,0,sizeof(touchState_t));
}


//--------------------------------------------------
// system touch event handler
//--------------------------------------------------


void wsApplication::onTouchEvent(
	unsigned x,
	unsigned y,
	u8 touch)
{
	#if DEBUG_TOUCH
		LOG("onTouchEvent(%d,%d,%02x) time=%d",x,y,touch,m_touch_state.time);
	#endif		

 	if (m_pTouchFocus)
	{
		m_pTouchFocus->m_state |= WIN_STATE_TOUCH_CHANGED;
		m_touch_state.state = touch;
		m_touch_state.x = x;
		m_touch_state.y = y;
		m_pTouchFocus->updateTouch(&m_touch_state);
	}
	else if (m_pTopWindow &&
			(touch & TOUCH_DOWN))
	{
		memset(&m_touch_state,0,sizeof(touchState_t));
		m_touch_state.state = touch;
		m_touch_state.x = x;
		m_touch_state.y = y;
		m_touch_state.time = CTimer::GetClockTicks() / 1000;
		m_touch_state.last_time = m_touch_state.time;
		m_pTopWindow->hitTest(x,y);
	}
}


//--------------------------------------------------
// timeSlice()
//--------------------------------------------------


void wsApplication::timeSlice()
{
	#ifdef DEBUG_UPDATE	
		if (debug_update)
		{
			delay(10);
			printf("================= update =====================\n");
			if (!m_pDC->getInvalid().isEmpty())
				print_rect("invalid",&m_pDC->getInvalid());
		}
	#endif
	
	if (m_pMouse)
		m_pMouse->UpdateCursor();
	
	// rate limit updates from the touch screen
	// to 60 per second
	
	if (m_pTouch)
	{
		if (CTimer::GetClockTicks() > m_lastTouchUpdate + CLOCKHZ/10)		// 60
		{
			m_pTouch->Update();
			m_lastTouchUpdate = CTimer::Get()->GetClockTicks();
		}
	}
	
	// call updateFrame 10 times per second
	// handling wrapping of the 100 mhz clock
	
	#define UPDATE_FRAOME_HUNDRETHS_OF_A_SECOND  10
	
	unsigned cur_time = CTimer::Get()->GetTicks();		// 100's of a second
	if (cur_time < m_update_frame_time ||
		cur_time > m_update_frame_time + UPDATE_FRAOME_HUNDRETHS_OF_A_SECOND)  
	{
		m_update_frame_time = cur_time;
		for (wsTopLevelWindow *p=m_pBottomWindow; p; p=p->m_pNextWindow)
		{
			p->updateFrame();
		}
	}
	
	// we do not call the base class update() method here ...
	// instead, we call update directly on each top level window
	// so that they are drawn in the right order (the stack order
	// as opposed to their instantiation order).
	//
	// This should *probably* be made into a generic behvavior, i.e.
	// each window's children should be in a modifyable zOrder with
	// a bringToTop() method.  Top level windows that are shown() are
	// then brought to the top, but so would be objects being dragged,
	// etc.

	for (wsTopLevelWindow *p=m_pBottomWindow; p; p=p->m_pNextWindow)
	{
		#ifdef DEBUG_UPDATE	
			if (debug_update)
			{
				delay(10);
				printf("---- top window ----\n");
			}
		#endif
		p->update();
	}
	
	// since we do not call our own base class update() method
	// we need to explicitly clear the state bits after the
	// first time through the loop.  They should not be reset
	// thereafter.
	
	clearBit(m_state,
		WIN_STATE_UPDATE |
		WIN_STATE_DRAW |
		WIN_STATE_REDRAW );
	
	// At this point all objects should be up to date and completely drawn.
	// We empty the DC's clipping region here, it will be expanded
	// as any objects are hidden or need to be redrawn in
	// response to the below (client) updateXXX() calls or
	// events
	
	m_pDC->validate();

	// Handle any time senstive touch (long click, repeat)
	// or drag-move events after validation.  
	
	if (m_pTouchFocus)
	{
		if (m_pTouchFocus->m_style & WIN_STYLE_TOUCH_TIMING_EVENTS)
		{
			m_pTouchFocus->updateTouch(&m_touch_state);
				// can call onUpdateClick(), onUpdateDrag(), etc
		}
	}

	#ifdef DEBUG_UPDATE	
		if (debug_update)
			debug_update--;
	#endif

	// dispatch any pending events to the top level window
	
	if (m_pFirstEvent && m_pTopWindow)
	{
		wsEvent *event = m_pFirstEvent;
		if (event->m_pNext)
		{
			m_pFirstEvent = event->m_pNext;
			m_pFirstEvent->m_pPrev = 0;
		}
		else
		{
			m_pFirstEvent = 0;
			m_pLastEvent = 0;
		}
		
		m_pTopWindow->handleEvent(event);
		delete event;
	}
}
