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
		m_align = ALIGN_CENTER_LEFT;
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
	m_pDC->invalidate(m_rect_abs);
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
			setBit(m_state,WIN_STATE_REDRAW);
			
		if (m_state & WIN_STATE_REDRAW)
		{
			draw();
			redraw = true;
			clearBit(m_state,WIN_STATE_REDRAW);
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
				setBit(p->m_state,WIN_STATE_REDRAW);
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
		#if DEBUG_TOUCH
			LOG("wsWindow::hitTest(%d,%d)=0x%08x",x,y,(u32)this);
		#endif
		if (m_style & WIN_STYLE_DRAG)
			onDragBegin();
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


// timing:
//
//     click happens when let up, no matter how long, unless long click
//     reoeat happens after 300 ms
//     long click happens after 800 ms
//     drag happens after 300 ms, or if they move the mouse out of the object


void wsWindow::updateTouch(touchState_t *touch_state)
	// after a hitTest, this is called by the application
	// on actual subsequent touch events, as well as 
	// from the main loop (very quickly) for windows
	// with timing based touch flags.  It is called
	// with the WIN_STATE_TOUCH_CHANGED bit set BEFORE
	// validation (on actual move events) and without
	// the bit set AFTER the validation.
	//
	// It generates the needed calls to onTouch(),
	// onClick(), onLongTouch() and onDragXXX()
	// which are NOT event handlers!
{
	u8 state = touch_state->state;
		
	//-------------------------------
	// non timing related changes
	//-------------------------------
	// if the mouse has strayed out of the object,
	// lose focus, 
	
	if (m_state & WIN_STATE_TOUCH_CHANGED)
	{
		#if DEBUG_TOUCH
			LOG("updateTouch(%d,%d,%02x)",touch_state->x,touch_state->y,touch_state->state);
		#endif

		m_state &= ~WIN_STATE_TOUCH_CHANGED;
		
		if (state & TOUCH_MOVE)
		{
			if (!(m_state & WIN_STATE_DRAGGING) &&
				!m_rect_abs.intersects(touch_state->x,touch_state->y))
			{
				#if DEBUG_TOUCH
					printf("updateTouch: touch strayed\n");
				#endif
				
				// if they dragged outside of area start dragging immediately
				
				if (m_style & WIN_STYLE_DRAG)
				{
					setBit(m_state,WIN_STATE_DRAGGING);
					touch_state->event_sent = true;
					onDragBegin();
				}
				else 	// regular button lost focus
				{
					clearBit(m_state,WIN_STATE_IS_TOUCHED);
					onTouch(0);
					getApplication()->setTouchFocus(0);
				}
				return;
			}
		}
	
		// otherwise, if the mouse has been released, generate
		// an onClick() event and lose focus
		
		if (state & TOUCH_UP)
		{
			#if DEBUG_TOUCH
				printf("updateTouch: touch up m_state=%08x\n",m_state);
			#endif
			
			clearBit(m_state,WIN_STATE_IS_TOUCHED);
			onTouch(0);
			
			if (m_state & WIN_STATE_DRAGGING)
			{
				clearBit(m_state,WIN_STATE_DRAGGING);
				onDragEnd();
			}
			else if (m_style & WIN_STYLE_CLICK)
			{
				if (!touch_state->event_sent)
					onClick();
			}
			getApplication()->setTouchFocus(0);
			return;
		}
		return;
	}
	
	
	//-------------------------------
	// handle time related events
	//-------------------------------
	// generate any events for pending touched object

	#define TOUCH_REPEAT_INC_INTERVAL    300		// the rate is increased every this many seconds
	#define TOUCH_NUM_INTERVALS          6
	#define TOUCH_RATE_MAX               5			// two hundred times per second max
	#define TOUCH_RATE_INC               (TOUCH_REPEAT_INC_INTERVAL - TOUCH_RATE_MAX) / TOUCH_NUM_INTERVALS;

	u32 now = CTimer::GetClockTicks() / 1000;
	
	if (m_style & WIN_STYLE_CLICK_REPEAT)
	{
		u32 interval_num = (now - touch_state->time) / TOUCH_REPEAT_INC_INTERVAL;
		if (interval_num > TOUCH_NUM_INTERVALS)
			interval_num = TOUCH_NUM_INTERVALS;

		if (interval_num)
		{
			u32 repeat_time = TOUCH_REPEAT_INC_INTERVAL - interval_num * TOUCH_RATE_INC;
			if (now > touch_state->last_time + repeat_time)
			{
				touch_state->last_time = now;
				onClick();
			}
		}
		return;
	}

	// We only do drags AFTER validation (after the update() loop,
	// when all objects have been redrawn, and at which time the
	// "invalid" DC rectangle is reset.  This is necessary so
	// that objects can invalidate their old rectangles for the
	// next time through the call to timeSlice().

	#define DRAG_CLICK_INTERVAL  400
		// if the thing is also clickable
		// we add a delay before we start moving
		// and abrogate the click event

	if (m_style & WIN_STYLE_DRAG)
	{
		if (!(m_state & WIN_STATE_DRAGGING))
		{
			u32 interval = (m_style & WIN_STYLE_CLICK) ?
				DRAG_CLICK_INTERVAL : 0;
			if (now > touch_state->time + interval)
			{
				m_state |= WIN_STATE_DRAGGING;
				touch_state->event_sent = true;
				onDragBegin();
			}
		}
		else if (touch_state->x != touch_state->last_x ||
				 touch_state->y != touch_state->last_y)
		{
			touch_state->last_x = touch_state->x;
			touch_state->last_y = touch_state->y;
			onDragMove();
		}

		return;
	}

	
	#define TOUCH_LONG_INTERVAL  800
	
	if (m_style & WIN_STYLE_CLICK_LONG &&
		!touch_state->event_sent &&
		now > touch_state->time + TOUCH_LONG_INTERVAL)
	{
		touch_state->event_sent = true;
		onLongClick();
		return;
	}

}


//-----------------------------------------
// base class state handlers
//-----------------------------------------
// These are not event handlers.
// They merely change the state of the object
// and return quickly.
//
// Note that the base class provides basic
// general behavior, and emits EVENT_TYPE_WINDOW
// events WIN_EVENT_CLOCK, and so on.  You must
// derive from this to emit other kinds of events
// like EVENT_TYPE_BUTTON

void wsWindow::onTouch(bool touched)
{
	redraw();
}


void wsWindow::onClick()
{
	getApplication()->addEvent(new wsEvent(
		EVT_TYPE_WINDOW,
		WIN_EVENT_CLICK,
		this ));
}

void wsWindow::onLongClick()
{
	getApplication()->addEvent(new wsEvent(
		EVT_TYPE_WINDOW,
		WIN_EVENT_LONG_CLICK,
		this ));
	
}

void wsWindow::onDblClick()	{}



//---------------------------------
// Generic dragging behavior
//---------------------------------

#define  DEBUG_DRAG  0
#define  DRAG_OUT_MARGIN  6
	// number of pixels that will show on screen
	// if they drag out right or bottom

void wsWindow::onDragBegin()
{
	wsApplication *app = getApplication();
	touchState_t *touch_state = app->getTouchState();
	
	// the drag could begin due to timing, or
	// the mouse being dragged off the object
	// in either case, we start by corraling
	// the x,y to within the object

	u16 x = touch_state->x;
	u16 y = touch_state->y;
	if (x < m_rect_abs.xs) x = m_rect_abs.xs;
	if (y < m_rect_abs.ys) y = m_rect_abs.ys;
	if (x > m_rect_abs.xe) x = m_rect_abs.xe;
	if (y > m_rect_abs.ye) y = m_rect_abs.ye;
	
	// make it relative to the object
	// and remember it

	touch_state->drag_x  = x - m_rect_abs.xs;
	touch_state->drag_y  = y - m_rect_abs.ys;

	#if DEBUG_DRAG
		LOG("onDragBegin(%d,%d) rect(%d,%d,%d,%d) drag_x=%d drag_y=%d",
			touch_state->x,
			touch_state->y,
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			touch_state->drag_x,
			touch_state->drag_y);	
	#endif
}


void wsWindow::onDragMove()
{
	wsApplication *app = getApplication();
	touchState_t *touch_state = app->getTouchState();
	wsRect rect(m_pParent->getClientRect());

	#if DEBUG_DRAG
		print_rect("client_rect",&rect);
	#endif
	
	// offset the abs coordinates to the
	// upper left corner of the object
	// constrained by absolute 0,0
	
	u16 x = touch_state->x;
	u16 y = touch_state->y;
	u16 drag_x = touch_state->drag_x;
	u16 drag_y = touch_state->drag_y;
	if (x < drag_x) x = drag_x;
	if (y < drag_y) y = drag_y;
	x -= drag_x;
	y -= drag_y;
	
	#if DEBUG_DRAG
		LOG("onDragMove(%d,%d) upper_left=%d,%d",
			touch_state->x,
			touch_state->y,
			x,y);
	#endif
	
	// corral the touch to the client area
	// and make it relative
	
	if (x < rect.xs) x = rect.xs;
	if (y < rect.ys) y = rect.ys;
	if (x > rect.xe - (DRAG_OUT_MARGIN-1)) x = rect.xe - (DRAG_OUT_MARGIN-1);
	if (y > rect.ye - (DRAG_OUT_MARGIN-1)) y = rect.ye - (DRAG_OUT_MARGIN-1);

	#if DEBUG_DRAG
		LOG("    constrained=%d,%d",x,y);
	#endif
	
	x -= rect.xs;
	y -= rect.ys;
	
	#if DEBUG_DRAG
		LOG("    relative=%d,%d",x,y);
	#endif
	
	move(x,y);
}


void wsWindow::onDragEnd()
{
	#if DEBUG_DRAG
		wsApplication *app = getApplication();
		touchState_t *touch_state = app->getTouchState();
		LOG("onDragEnd(%d,%d)",
			touch_state->x,
			touch_state->y);
	#endif
}
	
	
