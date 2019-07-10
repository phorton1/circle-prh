//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"
#include <circle/logger.h>
#define log_name  "wscont"


//-------------------------------------------------------
// wsStaticText
//-------------------------------------------------------

void wsStaticText::draw()
{
	wsColor bc = m_back_color;
	wsColor fc = m_fore_color;

	wsWindow::draw();
	
	//	m_pDC->setClip(m_clip_abs);
	//	m_pDC->fillFrame(
	//		m_rect_client.xs,
	//		m_rect_client.ys,
	//		m_rect_client.xe,
	//		m_rect_client.ye,
	//		bc);
	
	m_pDC->setFont(m_pFont);
	m_pDC->putText(
		bc,fc,
		m_rect_client,
		m_align,
		1,2,
		getText());
}



//-------------------------------------------------------
// wsButton
//-------------------------------------------------------

void wsButton::onTouch(bool touched)
	// onTouch primarily exists to set the WIN_STATE_REDRAW bit
	// which is done via calls to redraw()
{
	#if DEBUG_TOUCH
		printf("wsButton::onTouch(%d)\n",touched);
	#endif
	redraw();
}

void wsButton::onClick()
	// onClick() handlers must change their state
	// and also set WIN_STATE_REDRAW, as well as
	// generate any needed events.
{
	#if DEBUG_TOUCH
		printf("wsButton::onClick()\n",0);
	#endif

	if (m_button_style & BTN_STYLE_TOGGLE_VALUE)
		toggleBit(m_button_state, BTN_STATE_PRESSED);
	redraw();
	getApplication()->addEvent(new wsEvent(
		EVT_TYPE_BUTTON,
		BTN_EVENT_PRESSED,
		this ));
}



void wsButton::draw()
{
	wsColor bc = m_back_color;
	wsColor fc = m_fore_color;
	
	#if 0
		LOG("wsButton(%08x)::draw() m_button_style(0x%04x) m_state(0x%08x) m_button_state(0x%04x)",
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
	
	m_pDC->setClip(m_clip_client);
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
		m_pDC->setClip(m_clip_abs);
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
		m_pDC->setClip(m_clip_abs);
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



//-------------------------------------------------------
// wsCheckbox
//-------------------------------------------------------

void wsCheckbox::onTouch(bool touched)
{
	#if DEBUG_TOUCH
		printf("wsCheckbox::onTouch(%d)\n",touched);
	#endif
	if (touched)
		setBit(m_checkbox_state,CHB_STATE_PRESSED);
	else
		clearBit(m_checkbox_state,CHB_STATE_PRESSED);
	redraw();
}


void wsCheckbox::onClick()
{
	#if DEBUG_TOUCH
		printf("wsCheckbox::onClick()\n",0);
	#endif
	
	toggleBit(m_checkbox_state, CHB_STATE_CHECKED);
	redraw();
	
	getApplication()->addEvent(new wsEvent(
		EVT_TYPE_CHECKBOX,
		CHB_EVENT_VALUE_CHANGED,
		this ));
}



void wsCheckbox::draw()
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
	
	m_pDC->setClip(m_clip_client);
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

	u16 xoff = m_rect_abs.xs + 4;
	u16 yoff = m_rect_abs.ys + 4;
	for (u16 i=0; i<num_checkbox_coords; i++)
	{
		u16 x = xoff + checkbox_check_coords[i*2];
		u16 y = yoff + checkbox_check_coords[i*2+1];
		if (m_pDC->getClip().intersects(x,y))
			m_pDC->setPixel(x,y,color);
	}

	if (m_checkbox_style & CHB_STYLE_3D )
	{
		m_pDC->setClip(m_clip_abs);
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
		m_pDC->setClip(m_clip_abs);
		m_pDC->drawFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xs + CHECKBOX_SIZE - 1,
			m_rect_abs.ys + CHECKBOX_SIZE - 1,
			(m_checkbox_state & CHB_STATE_PRESSED) ?
				m_alt_back_color : m_alt_fore_color);
	}
}

