//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"
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

	resize(0,0,m_pScreen->GetWidth()-1, pScreen->GetHeight()-1);
	
	m_pDC = new wsDC(m_pScreen);
	m_pDC->setFont(wsFont8x14);
	
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
	#if 1
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

	
touchState_t *wsApplication::setTouchFocus(wsWindow *win)
	// called from hitTest on the the wsWindow object,
	// if any, that matched the starting x,y coordinates
{
	m_pTouchFocus = win;
	#if 1
		LOG("setTouchFocus(%08x)",(u32)win);
	#endif		
	
	if (win)
	{
		m_touch_state.start_state = m_touch_state.cur_state;
		m_touch_state.start_x = m_touch_state.cur_x;
		m_touch_state.start_y = m_touch_state.cur_y;
		m_touch_state.start_time = m_touch_state.cur_time;
	}
	else
	{
		memset(&m_touch_state,0,sizeof(touchState_t));
	}
	return &m_touch_state;
}


//--------------------------------------------------
// system touch event handler
//--------------------------------------------------

void wsApplication::onTouchEvent(
	unsigned x,
	unsigned y,
	u8 touch)
{
	m_touch_state.cur_state = touch;
	m_touch_state.cur_x = x;
	m_touch_state.cur_y = y;
	m_touch_state.cur_time = CTimer::GetClockTicks() / 1000;
	
	#if 1
		LOG("onTouchEvent(%d,%d,%02x) time=%d",x,y,touch,m_touch_state.cur_time);
	#endif		

 	if (m_pTouchFocus)
	{
		m_pTouchFocus->updateTouch(&m_touch_state);
	}
	else if (m_pTopWindow &&
			(touch & TOUCH_DOWN))
	{
		m_touch_state.start_state = 0;
		m_touch_state.start_x = 0;
		m_touch_state.start_y = 0;
		m_touch_state.start_time = 0;
		m_pTopWindow->hitTest(x,y);
	}
}


//--------------------------------------------------
// timeSlice()
//--------------------------------------------------

void wsApplication::timeSlice()
{
	if (m_pMouse)
		m_pMouse->UpdateCursor ();
	
	// rate limit updates from the touch screen
	// to 60 per second
	
	if (m_pTouch)
	{
		if (CTimer::Get()->GetClockTicks() > m_lastTouchUpdate + CLOCKHZ/10)		// 60
		{
			m_pTouch->Update();
			m_lastTouchUpdate = CTimer::Get()->GetClockTicks();
		}
	}
	
	// we do not call the base class update() method here ...
	// instead, we call update directly on each top level window
	// so that they are drawn in the right order (the stack order
	// as opposed to their instantiation order)

	for (wsTopLevelWindow *p=m_pBottomWindow; p; p=p->m_pNextWindow)
	{
		p->update(p->m_state & WIN_STATE_VISIBLE);
	}
	
	// we empty the clipping region here, it will be expanded
	// as any objects are hidden or need to be redrawn in
	// response to the below events
	
	m_pDC->validate();
	
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
