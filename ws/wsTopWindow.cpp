//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"
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
		
		// wsTopLevelWindow *pTop = pApp->getTopWindow();
		// if (pTop)
		//	pTop->m_state |= WIN_STATE_REDRAW;
	}
}

