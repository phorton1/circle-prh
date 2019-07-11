//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsControl.h"
#include <circle/logger.h>
#define log_name  "wscont"


//-------------------------------------------------------
// wsStaticText
//-------------------------------------------------------

void wsStaticText::onDraw()
{
	wsColor bc = m_back_color;
	wsColor fc = m_fore_color;

	wsWindow::onDraw();
	
	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
	m_pDC->setFont(m_pFont);
	m_pDC->putText(
		bc,fc,
		m_rect_client,
		m_align,
		1,2,
		getText());
}


