//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"
#include <circle/logger.h>
#define log_name  "wswin"



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
	m_rect(xs,ys,xe,ye)
	// m_rect_abs(xs,ys,xe,ye),
	// m_rect_client(xs,ys,xe,ye)
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

	if (m_pParent)
	{
		m_pParent->addChild(this);

		m_pDC = m_pParent->getDC();
		m_pFont = m_pParent->getFont();
		m_align = m_pParent->getAlign();
		m_fore_color = m_pParent->getForeColor();
		m_back_color = m_pParent->getBackColor();
	}
	else
	{
		m_fore_color = defaultWindowForeColor;
		m_back_color = defaultWindowBackColor;
		m_align = ALIGN_CENTER;
	}

	sizeWindow();
}


void wsWindow::sizeWindow()
{
	m_rect_abs.assign(m_rect);
	m_rect_client.assign(m_rect);
	
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
	
	m_clip_abs.assign(m_rect_abs);
	m_clip_client.assign(m_rect_client);
	
	if (m_pParent)
	{
		const wsRect &parent_rect = m_pParent->getClientRect();
		m_rect_abs.makeRelative(parent_rect);
		m_clip_abs.assign(m_rect_abs);
		m_clip_abs.intersect(parent_rect);

		m_rect_client.makeRelative(parent_rect);
		m_clip_client.assign(m_rect_client);
		m_clip_client.intersect(parent_rect);
	}
}


void wsWindow::sizeSelfAndChildren()
{
	sizeWindow();
	for (wsWindow *p = m_pFirstChild; p; p=p->m_pNextSibling)
	{
		p->sizeWindow();
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
	
	if (m_style & WIN_STYLE_2D)
	{
		// print_rect("2d frame",&m_rect_abs);
		m_pDC->setClip(m_clip_abs);
		m_pDC->drawFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			m_fore_color );
	}
	else if (m_style & WIN_STYLE_3D)
	{
		// print_rect("3d frame",&m_rect_abs);
		m_pDC->setClip(m_clip_abs);
		m_pDC->draw3DFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			windowFrameColors );
	}
	
	if (!(m_style & WIN_STYLE_TRANSPARENT))
	{
		// print_rect("fill client",&m_rect_abs);
		m_pDC->setClip(m_clip_client);
		m_pDC->fillFrame(
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
	redraw();
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
	sizeSelfAndChildren();
	redraw();
}

void wsWindow::move( u16 x, u16 y )
{
	u16 w = m_rect.xe - m_rect.xs + 1;
	u16 h = m_rect.ye - m_rect.ys + 1;
	m_rect.assign(x,y,x+w-1,y+h-1);
	sizeSelfAndChildren();
	redraw();
}



void wsWindow::show()
{
	if (!(m_state & WIN_STATE_VISIBLE))
	{
		m_state |= WIN_STATE_VISIBLE;
		redraw();
	}
}

void wsWindow::hide()
	// As it stands, the invalidation will cause all windows
	// windows under the closing window to be redrawn.
	//
	// As a result, we may be redrawing things that
	// don't need to be drawn if they are completely
	// or partially obscured by higher top level windows
	// or objects.
	//
	// We don't have a concept of the truly visible portions
	// of a window, which would require zorder maintenance
	// and a complicated clip_list, so I'm not doing it.
{
	if (m_state & WIN_STATE_VISIBLE)
	{
		m_pDC->invalidate(m_rect_abs);
		m_state &= ~WIN_STATE_VISIBLE;
	}		
}



void wsWindow::update(bool visible)
{
	// update self
	// and if drawing self, draw all children too
	
	bool redraw = false;
	const wsRect &invalid = m_pDC->getInvalid();
	
	if (visible && (m_state & WIN_STATE_VISIBLE))
	{
		if (m_rect_abs.intersects(invalid))
			m_state |= WIN_STATE_REDRAW;
			
		if (m_state & WIN_STATE_REDRAW)
		{
			draw();
			redraw = true;
			m_state &= ~WIN_STATE_REDRAW;
		}
	}
	
	// update children
	
	for (wsWindow *p = m_pFirstChild; p; p=p->m_pNextSibling)
	{
		if (redraw)
		{
			if (invalid.isEmpty() ||
				p->m_rect_abs.intersects(invalid))
			{
				p->m_state |= WIN_STATE_REDRAW;
			}
		}
		
		p->update(m_state & WIN_STATE_VISIBLE);
	}
}


wsWindow* wsWindow::hitTest(unsigned x, unsigned y)
{
	if ((m_style & WIN_STYLE_TOUCH) &&
		(m_state & WIN_STATE_VISIBLE) &&
		m_clip_abs.intersects(x,y))
	{
		setBit(m_state,WIN_STATE_IS_TOUCHED);	// | WIN_STATE_TOUCH_CHANGED;
		onTouch(1);
		#if 0
			LOG("wsWindow::hitTest(%d,%d)=0x%08x",x,y,(u32)this);
		#endif
		return this;
	}
	for (wsWindow *p = m_pFirstChild; p; p=p->m_pNextSibling)
	{
		wsWindow *found = p->hitTest(x,y);
		if (found)
		{
			#if 0
				LOG("wsWindow::hitTest(%d,%d) returning 0x%08x",x,y,(u32)found);
			#endif
			return found;
		}
	}
	return 0;
}



void wsWindow::updateTouch(touchState_t *touch_state)
	// called by the application on each subsequent
	// touch event after an initial succesfull hitTest(),
	// handles time and position based behaviors, generating
	// calls to onClick, onDblClick, etc.

{
	#if 0
		LOG("updateTouch(%d,%d,%02x)",touch_state->cur_x,touch_state->cur_y,touch_state->cur_state);
	#endif
	
	u8 state = touch_state->cur_state;
		
	// if the mouse has strayed out of the object,
	// lose focus, 
	
	if (state & TOUCH_MOVE)
	{
		if (!m_rect_abs.intersects(touch_state->cur_x,touch_state->cur_y))
		{
			#if 0
				printf("updateTouch: touch strayed\n");
			#endif
			clearBit(m_state,WIN_STATE_IS_TOUCHED);
			onTouch(0);
			getApplication()->setTouchFocus(0);
		}
	}
	
	// otherwise, if the mouse has been release, generate
	// an onClick() event and lose focus
	
	else if (!(state & TOUCH_DOWN))
	{
		#if 0
			printf("updateTouch: touch up\n");
		#endif
		
		clearBit(m_state,WIN_STATE_IS_TOUCHED);
		onTouch(0);
		if (m_style & WIN_STYLE_CLICK)
			onClick();
		getApplication()->setTouchFocus(0);
	}
}

	
	
