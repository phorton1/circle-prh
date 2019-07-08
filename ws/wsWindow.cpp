//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"
#include <circle/logger.h>
#include <circle/timer.h>
#define log_name  "ws"

#define delay(ms)   CTimer::Get()->MsDelay(ms)

//----------------------------------------------
// rectangle and object functions
//----------------------------------------------

void print_rect(const char *name,wsRect *rect)
{
	LOG("%s(%d,%d,%d,%d)",name,rect->xs,rect->ys,rect->xe,rect->ye);
}


//----------------------------------------------
// wsWindow
//----------------------------------------------

wsWindow::wsWindow(
		wsWindow *pParent,
		u16 id,
		u16 xs,
		u16 ys,
		u16 xe,
		u16 ye,
		u32 style) :
	m_id(id),
	m_style(style),
	m_rect(xs,ys,xe,ye),
	m_rect_abs(xs,ys,xe,ye),
	m_rect_client(xs,ys,xe,ye),
	m_rect_virt(0,0,xe-xs,ye-ys),
	m_rect_vis(0,0,xe-xs,ye-ys)
{
	// LOG("wsWindow(0x%08x,%d, %d,%d,%d,%d, 0x%08x)",(u32) this, id,xs,ys,xe,ye,style);
	
	m_pDC = 0;
	m_state = WIN_STATE_VISIBLE | WIN_STATE_REDRAW;
	m_pFont = 0;
	
	m_pParent = pParent;
	m_pPrevSibling = 0;
	m_pNextSibling = 0;
	m_pFirstChild  = 0;
	m_pLastChild   = 0;

	if (pParent)
	{
		m_pDC = pParent->getDC();
		m_pFont = pParent->getFont();
		m_align = pParent->getAlign();
		m_fore_color = pParent->getForeColor();
		m_back_color = pParent->getBackColor();
		
		m_rect_abs.makeRelative(pParent->getClentRect());
		m_rect_client.makeRelative(pParent->getClentRect());
		// print_rect("m_rect_abs",&m_rect_abs);
		
		if (m_style & WIN_STYLE_2D)
		{
			m_rect_client.xs++;
			m_rect_client.ys++;
			m_rect_client.xe--;
			m_rect_client.ye--;
		}
		else if (m_style & WIN_STYLE_3D)
		{
			m_rect_client.xs += 3;
			m_rect_client.ys += 3;
			m_rect_client.xe -= 3;
			m_rect_client.ye -= 3;
		}
		// print_rect("m_rect_client",&m_rect_client);
		
		pParent->addChild(this);
	}
	else
	{
		m_fore_color = defaultWindowForeColor;
		m_back_color = defaultWindowBackColor;
		m_align = ALIGN_CENTER;
	}
}

wsApplication *wsWindow::getApplication()
{
	wsWindow *p = this;
	while (p && !(p->m_style & WIN_STYLE_APPLICATION))
		p = p->m_pParent;
	if (!p)
	{
		LOG_ERROR("win(%08x) could not find application!",(u32)this);
	}
	return (wsApplication *) p;
}


void wsWindow::addChild(wsWindow *pWin)
{
	if (m_pLastChild)
	{
		m_pLastChild->m_pNextSibling = pWin;
		pWin->m_pPrevSibling = m_pLastChild;
	}
	else
	{
		m_pFirstChild = pWin;
	}
	m_pLastChild = pWin;
	m_numChildren++;
}


wsWindow *wsWindow::findChildByID(u16 id)
{
	if (id == m_id)
		return this;
	for (wsWindow *p = m_pFirstChild; p; p=p->m_pNextSibling)
	{
		wsWindow *found = p->findChildByID(id);
		if (found)
			return found;
	}
	return 0;
}




void wsWindow::draw()
{
	// draw self
	
	wsDC *pDC = getDC();
	if (m_style & WIN_STYLE_2D)
	{
		// print_rect("2d frame",&m_rect_abs);
		pDC->drawFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			m_fore_color );
	}
	else if (m_style & WIN_STYLE_3D)
	{
		// print_rect("3d frame",&m_rect_abs);
		pDC->draw3DFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			windowFrameColors );
	}
	
	if (!(m_style & WIN_STYLE_TRANSPARENT))
	{
		// print_rect("fill client",&m_rect_abs);
		pDC->fillFrame(
			m_rect_client.xs,
			m_rect_client.ys,
			m_rect_client.xe,
			m_rect_client.ye,
			m_back_color);
	}
}


void wsWindow::setText(const char *text)
{
	m_text = text;
	m_state |= WIN_STATE_REDRAW;
}


void wsWindow::setText(const CString &text)
{
	setText((const char *)text);
}


void wsWindow::resize(u16 xs, u16 ys, u16 xe, u16 ye )
	// currently only resizes this window
	// will need to resize/move children as well
	// which, in turn, requires more care with
	// the window rectangles.
{
	m_rect.assign(xs,ys,xe,ye);
	m_rect_abs.assign(xs,ys,xe,ye);
	m_rect_client.assign(xs,ys,xe,ye);
	m_rect_virt.assign(0,0,xe-xs,ye-ys);
	m_rect_vis.assign(0,0,xe-xs,ye-ys);
}


void wsWindow::show()
{
	if (!(m_state & WIN_STATE_VISIBLE))
	{
		m_state |= WIN_STATE_VISIBLE | WIN_STATE_REDRAW;
	}
}

void wsWindow::hide()
{
	if (m_state & WIN_STATE_VISIBLE)
	{
		m_state &= ~WIN_STATE_VISIBLE;
	}		
}


void wsWindow::update(bool visible)
{
	// update self
	// and if drawing self, draw all children too
	
	bool redraw = false;
	
	if (visible &&
		(m_state & WIN_STATE_VISIBLE) &&
		(m_state & WIN_STATE_REDRAW))
	{
		draw();
		redraw = true;
		m_state &= ~WIN_STATE_REDRAW;
	}
	
	// update children
	
	for (wsWindow *p = m_pFirstChild; p; p=p->m_pNextSibling)
	{
		if (redraw)
			p->m_state |= WIN_STATE_REDRAW;
		p->update(m_state & WIN_STATE_VISIBLE);
	}
}


wsWindow* wsWindow::hitTest(unsigned x, unsigned y)
{
	if ((m_style & WIN_STYLE_TOUCH) &&
		(m_state & WIN_STATE_VISIBLE) &&
		(x >= m_rect_abs.xs) &&
		(x <= m_rect_abs.xe) &&
		(y >= m_rect_abs.ys) &&
		(y <= m_rect_abs.ye))
	{
		m_state |= WIN_STATE_IS_TOUCHED | WIN_STATE_TOUCH_CHANGED;
		// LOG("wsWindow::hitTest(%d,%d)=0x%08x",x,y,(u32)this);
		return this;
	}
	for (wsWindow *p = m_pFirstChild; p; p=p->m_pNextSibling)
	{
		wsWindow *found = p->hitTest(x,y);
		if (found)
		{
			// LOG("wsWindow::hitTest(%d,%d) returning 0x%08x",x,y,(u32)found);
			return found;
		}
	}
	return 0;
}



//-------------------------------------------------------
// top level windows
//-------------------------------------------------------


wsTopLevelWindow::wsTopLevelWindow(wsApplication *pParent, u16 id, u16 xs, u16 ys, u16 xe, u16 ye, u32 style) :
	wsWindow(pParent,id,xs,ys,xe,ye,style | WIN_STYLE_TOP_LEVEL)
{
	m_pPrevWindow = 0;
	m_pNextWindow = 0;
	pParent->addTopLevelWindow(this);
}


wsWindow* wsTopLevelWindow::hitTest(unsigned x, unsigned y)
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
	// printf("wsTopLevelWindow::hitTest() found=%08x\n",found);
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
		wsTopLevelWindow *pTop = pApp->getTopWindow();
		if (pTop)
			pTop->m_state |= WIN_STATE_REDRAW;
	}
}


//-------------------------------------------------------
// wsStaticText
//-------------------------------------------------------

void wsStaticText::draw()
{
	wsDC *pDC = getDC();
	wsColor bc = m_back_color;
	wsColor fc = m_fore_color;
	pDC->fillFrame(
		m_rect_client.xs,
		m_rect_client.ys,
		m_rect_client.xe,
		m_rect_client.ye,
		bc);
	pDC->setFont(m_pFont);
	pDC->putText(
		bc,fc,
		&m_rect_client,
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
		
		m_state |= WIN_STATE_REDRAW;
	}
	if (m_state & WIN_STATE_REDRAW)
	{
		draw();
		m_state &= ~WIN_STATE_REDRAW;
	}
}


void wsButton::draw()
{
	wsDC *pDC = getDC();
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
	
    if ( !(m_button_style & BTN_STYLE_NO_FILL))
	{
        pDC->fillFrame(
			m_rect_client.xs,
			m_rect_client.ys,
			m_rect_client.xe,
			m_rect_client.ye,
			bc);
	}
	
	pDC->setFont(m_pFont);
	pDC->putText(
		bc,fc,
		&m_rect_client,
		m_align,
		1,2,
		getText());

	if ( m_button_style & BTN_STYLE_3D )
	{
		// LOG("draw3dframe() pressed=%d",m_button_state & BTN_STATE_PRESSED ? 1 : 0);
	    pDC->draw3DFrame(
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
		pDC->drawFrame(
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
		
		m_state |= WIN_STATE_REDRAW;
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
	wsDC *pDC = getDC();
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
	
    if ( !(m_checkbox_style & CHB_STYLE_NO_FILL))
	{
        pDC->fillFrame(
			m_rect_client.xs,
			m_rect_client.ys,
			m_rect_client.xe,
			m_rect_client.ye,
			getParent()->getBackColor());
        pDC->fillFrame(
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
		pDC->putText(
			getParent()->getBackColor(),
			getParent()->getForeColor(),
			&rtext,
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
		pDC->setPixel(x,y,color);
	}

	if ( m_checkbox_style & CHB_STYLE_3D )
	{  
	   pDC->draw3DFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xs + CHECKBOX_SIZE - 1,
			m_rect_abs.ys + CHECKBOX_SIZE - 1,
			(m_checkbox_state & CHB_STATE_PRESSED) ?
			buttonPressedFrameColors : buttonReleasedFrameColors);
	}
	else if ( m_checkbox_style & BTN_STYLE_2D )
	{
		pDC->drawFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xs + CHECKBOX_SIZE - 1,
			m_rect_abs.ys + CHECKBOX_SIZE - 1,
			(m_checkbox_state & CHB_STATE_PRESSED) ?
				m_alt_back_color : m_alt_fore_color);
	}
}

