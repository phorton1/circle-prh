//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsApp_h
#define _wsApp_h

#include "wsTopWindow.h"
#include <circle/input/mouse.h>
#include <circle/input/touchscreen.h>


//------------------------------------
// the application object
//------------------------------------

class wsApplication : public wsWindow
{
	public:
		
		~wsApplication();
		wsApplication();

		// You must call Initialize and provide a Create() method.
		// The Create() method will be called to allow you to create
		// the initial set of windows and controls, thereafter you
		// need only call timeSlice() in a loop.
		
		void Initialize(
			CScreenDeviceBase *pScreen,
			CTouchScreenBase  *pTouch,
			CMouseDevice      *pMouse);
		
		void Create();
			// You must provide wsApplication::Create()
			
		void timeSlice();
			// it's called timeSlice() to prevent confusion
			// with the wsWindows::update() functions. 

		wsTopLevelWindow *getTopWindow() const { return m_pTopWindow; }
		touchState_t *getTouchState()	  	   { return &m_touch_state; }
		
		// public to wsWindows
		// not intended for client use
		
		void setTouchFocus(wsWindow *win);
			// called from hitTest on the the wsWindow object,
			// if any, that matched the starting x,y coordinates
		void addTopLevelWindow(wsTopLevelWindow *pWindow);
		void removeTopLevelWindow(wsTopLevelWindow *pWindow);
		
		void addEvent(wsEvent *event);
		
	private:
		
		wsEvent *m_pFirstEvent;
		wsEvent *m_pLastEvent;
		
		wsWindow *m_pTouchFocus;
		touchState_t m_touch_state;
		u32 m_lastTouchUpdate;
		
		wsTopLevelWindow *m_pBottomWindow;
		wsTopLevelWindow *m_pTopWindow;
		
		CScreenDeviceBase *m_pScreen;
		CTouchScreenBase  *m_pTouch;
		CMouseDevice      *m_pMouse;
		
		void mouseEventHandler(
			TMouseEvent event,
			unsigned buttons,
			unsigned x,
			unsigned y);
		static void mouseEventStub(
			void *pThis,
			TMouseEvent event,
			unsigned buttons,
			unsigned x,
			unsigned y);
	
		void touchEventHandler(
			TTouchScreenEvent event,
			unsigned id,
			unsigned x,
			unsigned y);
		static void touchEventStub(
			void *pThis,
			TTouchScreenEvent event,
			unsigned id,
			unsigned x,
			unsigned y);
		
		void onTouchEvent(
			unsigned x,
			unsigned y,
			u8 touch);
		
		
};


#endif  // !_wsWApp_h
