//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsTopWindow.h"
#include "wsApp.h"

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
		((wsApplication *)m_pParent)->setTouchFocus(found);
	}
	return found;
}


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
		((wsApplication *)m_pParent)->addTopLevelWindow(this);
	}
}

void wsTopLevelWindow::hide()
{
	if (m_state & WIN_STATE_VISIBLE)
	{
		wsApplication *pApp = ((wsApplication *)m_pParent);
		wsWindow::hide();
		pApp->removeTopLevelWindow(this);
	}
}

