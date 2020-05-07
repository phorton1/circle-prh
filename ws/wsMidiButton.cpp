//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsMidiButton.h"
#include "wsApp.h"
#include "wsEvent.h"
#include <circle/logger.h>

#define log_name  "midi_button"


wsMidiButton::wsMidiButton(
		wsWindow *pParent,
		u16 id,
		const char *text,
		s32 xs,
		s32 ys,
		s32 xe,
		s32 ye) :
	wsControl(pParent,id,xs,ys,xe,ye,
		WIN_STYLE_TOUCH |
		WIN_STYLE_CLICK |
		WIN_STYLE_3D )
{
	LOG("ctor(%s)",text);

	// base class
	
	m_align = ALIGN_CENTER;
	m_fore_color = defaultButtonForeColor;
	m_back_color = defaultButtonReleasedColor;
	m_text = text;

	// this class
	
	m_pressed = 0;
	m_button_state = 0;
	m_pressed_back_color = defaultButtonPressedColor;
	m_pressed_fore_color = m_fore_color;

}


void wsMidiButton::onUpdateTouch(bool touched)
{
	// LOG("onUpdateTouch(%s) touched=%d",getText(),touched);
	m_pressed = touched;
	setBit(m_state,WIN_STATE_DRAW);

	if (touched)
	{
		getApplication()->addEvent(new wsEvent(
			EVT_TYPE_BUTTON,
			BTN_EVENT_PRESSED,
			this ));
	}
}


//	void wsMidiButton::onUpdateClick()
//	{
//		#if 1  // DEBUG_TOUCH
//			printf("wsMidiButton(%08x)::onUpdateClick()\n",(u32)this);
//		#endif
//	
//		setBit(m_state,WIN_STATE_DRAW);
//		
//	}



void wsMidiButton::onDraw()
{
	// using member "m_text" causes funny memory problems
	// use getText() instead
	
	// LOG("onDraw(%s) m_pressed(%d) m_button_state(0x%04x)",getText(),m_pressed,m_button_state);

	wsColor bc = m_pressed ? m_pressed_back_color : m_back_color;
	wsColor fc = m_pressed ? m_pressed_fore_color : m_fore_color;
	
	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
	m_pDC->fillFrame(
		m_rect_client.xs,
		m_rect_client.ys,
		m_rect_client.xe,
		m_rect_client.ye,
		bc);
	
	m_pDC->setFont(m_pFont);
	m_pDC->putText(
		bc,fc,
		m_rect_client,
		m_align,
		1,2,
		getText());

	m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);
	m_pDC->draw3DFrame(
		m_rect_abs.xs,
		m_rect_abs.ys,
		m_rect_abs.xe,
		m_rect_abs.ye,
		m_pressed ?
			buttonPressedFrameColors :
			buttonReleasedFrameColors);
}

