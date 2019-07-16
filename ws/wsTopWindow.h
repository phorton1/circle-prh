//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsTopWindow_h
#define _wsTopWindow_h

#include "wsWindow.h"

//-------------------------------------------
// top level window
//-------------------------------------------

class wsTopLevelWindow : public wsWindow
{
	public:
	
		wsTopLevelWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye, u32 style=0);
		~wsTopLevelWindow() {}
		
		virtual wsApplication *getApplication() const
			{ return (wsApplication *) m_pParent; }

		void show();
		void hide();

		// not intended for public use
		
		virtual u32 handleEvent(wsEvent *event);
		
	protected:
		
		friend class wsApplication;
		
		virtual wsWindow *hitTest(s32 x, s32 y);
		
		
		wsTopLevelWindow *m_pPrevWindow;
		wsTopLevelWindow *m_pNextWindow;
		
};


#endif  // !_wsTopWindow_h
