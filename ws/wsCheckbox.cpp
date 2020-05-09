//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsCheckbox.h"
#include "wsApp.h"
#include "wsEvent.h"

#include <circle/logger.h>
#define log_name  "wscont"


//-------------------------------------------------------
// wsCheckbox
//-------------------------------------------------------
// see notes on wsButton methods

void wsCheckbox::onUpdateTouch(bool touched)
{
	#if DEBUG_TOUCH
		printf("wsCheckbox(%08x)::onTouch(%d)\n",(u32)this,touched);
	#endif
	if (touched)
		setBit(m_checkbox_state,CHB_STATE_PRESSED);
	else
		clearBit(m_checkbox_state,CHB_STATE_PRESSED);
	setBit(m_state,WIN_STATE_DRAW);
}


void wsCheckbox::onUpdateClick()
{
	#if DEBUG_TOUCH
		printf("wsCheckbox(%08x)::onUpdateClick()\n",(u32)this);
	#endif
	
	toggleBit(m_checkbox_state, CHB_STATE_CHECKED);
	setBit(m_state,WIN_STATE_DRAW);
	
	getApplication()->addEvent(new wsEvent(
		EVT_TYPE_CHECKBOX,
		EVENT_VALUE_CHANGED,
		this ));
}



void wsCheckbox::onDraw()
{
	wsColor bc = m_back_color;
	wsColor fc = m_fore_color;

    if (m_checkbox_state & CHB_STATE_PRESSED)
    {
		if (m_checkbox_style & CHB_STYLE_TOGGLE_COLORS)
		{
		   bc = m_fore_color;
		   fc = m_back_color;
		}
		else if (m_checkbox_style & CHB_STYLE_USE_ALTERNATE_COLORS )
		{
		   bc = m_alt_back_color;
		   fc = m_alt_fore_color;
		}
    }
	
	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
    if (!(m_style & WIN_STYLE_TRANSPARENT))
	{
        m_pDC->fillFrame(					// fill the checkbox background
			m_rect_client.xs,
			m_rect_client.ys,
			m_rect_client.xs + CHECKBOX_SIZE - 1,
			m_rect_client.ys + CHECKBOX_SIZE - 1,
			bc);
	}
	
	wsColor color = m_checkbox_state & CHB_STATE_CHECKED ?
		fc : bc;

	s32 xoff = m_rect_abs.xs + 4;
	s32 yoff = m_rect_abs.ys + 4;
	for (u16 i=0; i<num_checkbox_coords; i++)
	{
		s32 x = xoff + checkbox_check_coords[i*2];
		s32 y = yoff + checkbox_check_coords[i*2+1];
		if (m_pDC->getClip().intersects(x,y))
			m_pDC->setPixel(x,y,color);
	}

	if (m_checkbox_style & CHB_STYLE_3D )
	{
		m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);
	    m_pDC->draw3DFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xs + CHECKBOX_SIZE - 1,
			m_rect_abs.ys + CHECKBOX_SIZE - 1,
			(m_checkbox_state & CHB_STATE_PRESSED) ?
			buttonPressedFrameColors : buttonReleasedFrameColors);
	}
	else if (m_checkbox_style & CHB_STYLE_2D )
	{
		m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);
		m_pDC->drawFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xs + CHECKBOX_SIZE - 1,
			m_rect_abs.ys + CHECKBOX_SIZE - 1,
			(m_checkbox_state & CHB_STATE_PRESSED) ?
				m_alt_back_color : m_alt_fore_color,
			m_frame_width);
	}
}

