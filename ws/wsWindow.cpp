//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"
#include <circle/logger.h>
#define log_name  "ws"


//----------------------------------------------
// rectangle and object functions
//----------------------------------------------

void wsRect::print(const char *name)
{
	printf("%s(%d,%d,%d,%d)\n",name,xs,ys,xe,ye);
}


void wsObject::addChild(wsObject *object, wsObject *pAddBefore /* =0 */)
{
	if (m_pLastChild)
	{
		m_pLastChild->m_pNextSibling = object;
		object->m_pPrevSibling = m_pLastChild;
	}
	else
	{
		m_pFirstChild = object;
	}
	m_pLastChild = object;
	m_numChildren++;
}




//----------------------------------------------
// wsWindowBase
//----------------------------------------------

wsWindowBase::wsWindowBase(
		wsWindowBase *pParent,
		u16 id,
		u16 xs,
		u16 ys,
		u16 xe,
		u16 ye,
		u32 style) :
	wsEventHandler(pParent),
	m_id(id),
	m_style(style),
	m_rect(xs,ys,xe,ye),
	m_rect_abs(xs,ys,xe,ye),
	m_rect_client(xs,ys,xe,ye),
	m_rect_virt(0,0,xe-xs,ye-ys),
	m_rect_vis(0,0,xe-xs,ye-ys)
{
	LOG("wsWindowBase(0x%08x,%d, %d,%d,%d,%d, 0x%08x)",(u32) this, id,xs,ys,xe,ye,style);
	
	m_pDC = 0;
	m_state = 0;
	m_pFont = 0;
	
	if (pParent)
	{
		m_pDC = pParent->getDC();
		m_pFont = pParent->getFont();
		m_align = pParent->getAlign();
		m_fore_color = pParent->getForeColor();
		m_back_color = pParent->getBackColor();
		
		m_rect_abs.makeRelative(pParent->getClentRect());
		m_rect_client.makeRelative(pParent->getClentRect());
		m_rect_abs.print("m_rect_abs");
		
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
		m_rect_client.print("m_rect_client");
		pParent->addChild(this);
	}
	else
	{
		m_fore_color = defaultWindowForeColor;
		m_back_color = defaultWindowBackColor;
		m_align = ALIGN_CENTER;
	}
}


wsDC *wsWindowBase::getDC() const
{
	// m_pDC->setFont(m_pFont);
	// m_pDC->setForeColor(m_fore_color);
	// m_pDC->setBackColor(m_back_color);
	return m_pDC;
}


// virtual
void wsWindowBase::draw()
{
	// draw self
	
	wsDC *pDC = getDC();
	if (m_style & WIN_STYLE_2D)
	{
		m_rect_abs.print("2d frame");
		pDC->drawFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			m_fore_color );
	}
	else if (m_style & WIN_STYLE_3D)
	{
		m_rect_abs.print("3d frame");
		pDC->draw3DFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			windowFrameColors );
	}
	
	if (!(m_style & WIN_STYLE_TRANSPARENT))
	{
		m_rect_abs.print("fill client");
		pDC->fillFrame(
			m_rect_client.xs,
			m_rect_client.ys,
			m_rect_client.xe,
			m_rect_client.ye,
			m_back_color);
	}
	
	// draw children
	
	for (wsWindowBase *p = (wsWindowBase *) getFirstChild(); p; p = (wsWindowBase *) p->getNextSibling())
	{
		p->draw();
	}
}


void wsWindowBase::setText(const char *text)
{
	printf("setText(const char *\"%s\")\n",text);
	m_text = text;	
	printf("result=\"%s\"\n",(const char *)m_text);
}
void wsWindowBase::setText(const CString &text)
{
	printf("setText(const CString *\"%s\")\n",(const char *)text);
	m_text = text;
	printf("result=\"%s\"\n",(const char *)m_text);
}

void wsWindowBase::resize(u16 xs, u16 ys, u16 xe, u16 ye )
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



wsWindow::wsWindow(wsWindowBase *pParent, u16 id, u16 xs, u16 ys, u16 xe, u16 ye, u32 style) :
	wsWindowBase(pParent,id,xs,ys,xe,ye,style)
{
}
		
		

wsTopLevelWindow::wsTopLevelWindow(wsApplication *pParent, u16 id, u16 xs, u16 ys, u16 xe, u16 ye, u32 style) :
	wsWindow(pParent,id,xs,ys,xe,ye,style)
{
	m_zorder = 0;
}


wsEventResult wsTopLevelWindow::handleEvent(wsEvent *event)
{
	return 0;
}





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



void wsButton::draw()
{
	wsDC *pDC = getDC();
	wsColor bc = m_back_color;
	wsColor fc = m_fore_color;
	
    if (m_button_state & BTN_STATE_PRESSED)
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
	   pDC->draw3DFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			(m_button_state & BTN_STATE_PRESSED) ?
			buttonPressedFrameColors : buttonReleasedFrameColors);
	}
	else if ( m_button_style & BTN_STYLE_2D )
	{
		pDC->drawFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			(m_button_state & BTN_STATE_PRESSED) ?
				m_alt_back_color : m_alt_fore_color);
	}
}



#define CHECKBOX_SIZE   20
#define CHECKBOX_TEXT_X_OFFSET  30
#define CHECKBOX_TEXT_Y_OFFSET  3


void wsCheckbox::draw()
	// justification needs some work
	// - horzontal and vertical centering
	// - switch box and text for right justified
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

