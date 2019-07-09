//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"
#include <circle/logger.h>
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
	m_lastTouchUpdate = 0;
	m_pTouchFocus = 0;
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
	// LOG("mouse event(%d,%d,%d,%d)",event,buttons,x,y);
	if (event == MouseEventMouseDown ||
		event == MouseEventMouseUp)
		onTouchEvent(x,y,event == MouseEventMouseDown);
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
	// LOG("touch event(%d,%d,%d,%d)",event,id,x,y);
	if (event == TouchScreenEventFingerDown ||
		event==TouchScreenEventFingerUp)
		onTouchEvent(x,y,event == TouchScreenEventFingerDown);
}


void wsApplication::onTouchEvent(
	unsigned x,
	unsigned y,
	bool down)
{
	// LOG("onTouchEvent(%d,%d,%d) m_pTouchFocus=%08x",x,y,down,(u32)m_pTouchFocus);
	if (down)
	{
		if (m_pTopWindow)
		{
			m_pTopWindow->hitTest(x,y);
		}
	}
	else if (m_pTouchFocus)
	{
		// LOG("Clearing touch(%08x)",(u32)m_pTouchFocus);
		m_pTouchFocus->m_state &= ~WIN_STATE_IS_TOUCHED;
		m_pTouchFocus->m_state |= WIN_STATE_TOUCH_CHANGED;
		m_pTouchFocus->update(true);
		m_pTouchFocus = 0;
	}
}


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
