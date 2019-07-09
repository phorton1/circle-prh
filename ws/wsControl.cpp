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

	m_pDC->setClip(m_clip_abs);
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
}



//-------------------------------------------------------
// wsButton
//-------------------------------------------------------

void wsButton::update(bool visible)
{
	if (m_state & WIN_STATE_TOUCH_CHANGED)
	{
		// LOG("wsButton(%08x) TOUCH_CHANGED IS_TOUCH=%d",
		// 	(u32)this,m_state & WIN_STATE_IS_TOUCHED ? 1 : 0);
		
		m_state &= ~WIN_STATE_TOUCH_CHANGED;
		if (m_button_style & BTN_STYLE_TOGGLE_VALUE)
		{
			if (m_state & WIN_STATE_IS_TOUCHED)
			{
				if (m_button_state & BTN_STATE_PRESSED)
				{
					// printf("toggle button up\n");
					m_button_state &= ~BTN_STATE_PRESSED;
				}
				else
				{
					// printf("toggle button down\n");
					m_button_state |= BTN_STATE_PRESSED;
				}
			}
		}
		else if (m_state & WIN_STATE_IS_TOUCHED)
		{
			// printf("button down\n");
			m_button_state |= BTN_STATE_PRESSED;
		}
		else
		{
			// printf("button up\n");
			m_button_state &= ~BTN_STATE_PRESSED;
		}
		
		// generate the event on the touch up
		
		if (!(m_state & WIN_STATE_IS_TOUCHED))
		{
			getApplication()->addEvent(new wsEvent(
				EVT_TYPE_BUTTON,
				BTN_EVENT_PRESSED,
				this ));
		}
		
		redraw();	// sets WIN_STATE_REDRAW and invalidates the rect
	}
	if (m_state & WIN_STATE_REDRAW)
	{
		draw();
		m_state &= ~WIN_STATE_REDRAW;
	}
}


void wsButton::draw()
{
	wsColor bc = m_back_color;
	wsColor fc = m_fore_color;
	
	// LOG("wsButton(%08x)::draw() m_button_style(0x%04x) m_state(0x%08x) m_button_state(0x%04x)",
	// 	   (u32) this,
	// 	   m_button_style,
	// 	   m_state,
	// 	   m_button_state);
	
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
    if ( !(m_button_style & BTN_STYLE_NO_FILL))
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

	if ( m_button_style & BTN_STYLE_3D )
	{
		// LOG("draw3dframe() pressed=%d",m_button_state & BTN_STATE_PRESSED ? 1 : 0);
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
	else if ( m_button_style & BTN_STYLE_2D )
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

void wsCheckbox::update(bool visible)
{
	if (m_state & WIN_STATE_TOUCH_CHANGED)
	{
		// LOG("wsCheckbox(%08x) TOUCH_CHANGED IS_TOUCH=%d",
		// 	(u32)this,m_state & WIN_STATE_IS_TOUCHED ? 1 : 0);
		
		m_state &= ~WIN_STATE_TOUCH_CHANGED;
		if (m_state & WIN_STATE_IS_TOUCHED)
		{
			m_checkbox_state |= CHB_STATE_PRESSED;
			if (m_checkbox_state & CHB_STATE_CHECKED)
			{
				// printf("uncheck box\n");
				m_checkbox_state &= ~CHB_STATE_CHECKED;
			}
			else
			{
				// printf("check the box\n");
				m_checkbox_state |= CHB_STATE_CHECKED;
			}
		}
		else
		{
			m_checkbox_state &= ~CHB_STATE_PRESSED;
		
			// generate the event on the touch up
			
			getApplication()->addEvent(new wsEvent(
				EVT_TYPE_CHECKBOX,
				CHB_EVENT_VALUE_CHANGED,
				this ));
		}
		
		redraw();	// sets WIN_STATE_REDRAW and invalidates the rect
	}
	if (m_state & WIN_STATE_REDRAW)
	{
		draw();
		m_state &= ~WIN_STATE_REDRAW;
	}
}


#define CHECKBOX_SIZE   20
#define CHECKBOX_TEXT_X_OFFSET  30
#define CHECKBOX_TEXT_Y_OFFSET  3


void wsCheckbox::draw()
	// I don't really like having text associated with checkboxes
	//
	// It is not clear how to lay it out, and the hit test area
	// is not obvious to the user.  It seems that for right
	// justification the text should come befor the box.
	// What about vertical justifications.
	// Thus munging justification with layout direction.
	//
	// A better encapsulation would be to have a wsCheckBoxWithText
	// object that has a separate static text object with well
	// defined layout semantics.
	//
	// As it stands right now, if you call setText() on the object
	// on each CHB_EVENT_VALUE_CHANGED event, the text toggles on
	// and off with the checkbox, so you can only see the "on" text.
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
	
    if ( !(m_checkbox_style & CHB_STYLE_NO_FILL))
	{
        m_pDC->fillFrame(
			m_rect_client.xs,
			m_rect_client.ys,
			m_rect_client.xe,
			m_rect_client.ye,
			getParent()->getBackColor());
        m_pDC->fillFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xs + CHECKBOX_SIZE - 1,
			m_rect_abs.ys + CHECKBOX_SIZE - 1,
			bc);
	}
	
	if (m_text.GetLength())
	{
		wsRect rtext(m_rect_abs);
		rtext.xs += CHECKBOX_TEXT_X_OFFSET;
		rtext.ys += CHECKBOX_TEXT_Y_OFFSET;
		m_pDC->putText(
			getParent()->getBackColor(),
			getParent()->getForeColor(),
			rtext,
			m_align,
			1,2,
			getText());
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

	if ( m_checkbox_style & CHB_STYLE_3D )
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
	else if ( m_checkbox_style & BTN_STYLE_2D )
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

