//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsButton.h"
#include "wsApp.h"
#include "wsEvent.h"

#include <circle/logger.h>
#define log_name  "button"


wsButton::wsButton(
		wsWindow *pParent,
		u16 id,
		const char *text,
		s32 xs,
		s32 ys,
		s32 xe,
		s32 ye,
		u16 bstyle,
		u32 addl_wstyle) :
	wsControl(pParent,id,xs,ys,xe,ye,
		addl_wstyle |
		WIN_STYLE_TOUCH |
		WIN_STYLE_CLICK | (
		(bstyle & BTN_STYLE_3D) ? WIN_STYLE_3D :
		(bstyle & BTN_STYLE_2D) ? WIN_STYLE_2D : 0))
{
	// base class
	
	m_align = ALIGN_CENTER;
	m_fore_color = defaultButtonForeColor;
	m_back_color = defaultButtonReleasedColor;
	if (text)
		m_text = text;

	// this class
	
	m_button_state = 0;
	m_button_style = bstyle;
	m_alt_back_color = defaultButtonPressedColor;
	m_alt_fore_color = m_fore_color;
}


void wsButton::onUpdateTouch(bool touched)
{
	#if DEBUG_TOUCH
		printf("wsButton(%08x)::onUpdateTouch(%d)\n",(u32)this,touched);
	#endif
	setBit(m_state,WIN_STATE_DRAW);
}



void wsButton::onUpdateClick()
{
	#if 1  // DEBUG_TOUCH
		printf("wsButton(%08x)::onUpdateClick()\n",(u32)this);
	#endif

	if (m_button_style & BTN_STYLE_TOGGLE_VALUE)
		toggleBit(m_button_state, BTN_STATE_PRESSED);

	setBit(m_state,WIN_STATE_DRAW);
	
	getApplication()->addEvent(new wsEvent(
		EVT_TYPE_BUTTON,
		BTN_EVENT_PRESSED,
		this ));
}



void wsButton::onDraw()
{
	wsColor bc = m_back_color;
	wsColor fc = m_fore_color;
	
	#if 0
		LOG("wsButton(%08x)::onDraw() m_button_style(0x%04x) m_state(0x%08x) m_button_state(0x%04x)",
			(u32) this,
			m_button_style,
			m_state,
			m_button_state);
	#endif
	
	bool pressed =
		m_state & WIN_STATE_IS_TOUCHED ||
		m_button_state & BTN_STATE_PRESSED;
		
    if (pressed)
    {
		if (m_button_style & BTN_STYLE_TOGGLE_COLORS)
		{
		   bc = m_fore_color;
		   fc = m_back_color;
		}
		else if (m_button_style & BTN_STYLE_USE_ALTERNATE_COLORS )
		{
		   bc = m_alt_back_color;
		   fc = m_alt_fore_color;
		}
    }
	
	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
    if ( !(m_style & WIN_STYLE_TRANSPARENT))
	{
        m_pDC->fillFrame(
			m_rect_client.xs,
			m_rect_client.ys,
			m_rect_client.xe,
			m_rect_client.ye,
			bc);
	}
	
	m_pDC->setFont(m_pFont);
	m_pDC->putText(
		bc,fc,
		m_rect_client,
		m_align,
		1,2,
		getText());

	if (m_button_style & BTN_STYLE_3D)
	{
		m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);
	    m_pDC->draw3DFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			pressed ?
				buttonPressedFrameColors :
				buttonReleasedFrameColors);
	}
	else if (m_button_style & BTN_STYLE_2D )
	{
		m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);
		m_pDC->drawFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			pressed ? 
				m_alt_back_color :
				m_alt_fore_color);
	}
}

