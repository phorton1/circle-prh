//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"
#include <circle/logger.h>
#define log_name  "wsapp"


wsApplication::~wsApplication()
{
}

wsApplication::wsApplication(
		CScreenDeviceBase *pScreen,
		CTouchScreenBase  *pTouch,
		CMouseDevice      *pMouse) :
	wsWindow(0,0, 0,0,pScreen->GetWidth()-1, pScreen->GetHeight()-1, 0),
	m_pScreen(pScreen),
    m_pTouch(pTouch),
    m_pMouse(pMouse)
{
	LOG("ctor",0);
	m_pDC = new wsDC(pScreen);
	m_pDC->setFont(wsFont8x14);
	setFont(wsFont8x14);
}
		