//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"
#include "wsApp.h"
#include "wsEvent.h"

#include <circle/logger.h>
#define log_name  "wswin"




#ifdef DEBUG_UPDATE
	#define DBG_UPDATE(level, format, ...)		\
		if (debug_update)						\
		{										\
			delay(10);							\
			debug_indent(level);			 	\
			printf(format,__VA_ARGS__);			\
		}

	int debug_update = 1;
		// set to 1 to show the first update, 0 to not
	int debug_update_level = 0;
	void debugUpdate(int num)
	{
		debug_update = num;
	}
	
	void debug_indent(int addl)
	{
		for (int i=0; i<debug_update_level + addl; i++)
			printf("  ");
	}
	#define INC_UPDATE_LEVEL()		debug_update_level++;
	#define DEC_UPDATE_LEVEL()		debug_update_level--;
	
#else
	#define DBG_UPDATE(l,f,...)
	#define INC_UPDATE_LEVEL()	
	#define DEC_UPDATE_LEVEL()	
#endif



//----------------------------------------------
// wsWindow
//----------------------------------------------

#include <circle/synchronize.h>

wsWindow::~wsWindow()
{
	DisableIRQs();	// in synchronize.h
	
	// LOG("wsWindow(%X) dtor",(u32)this);
    while (m_pFirstChild)
	{
		printf("deleting child 0x%X\n",(u32) m_pFirstChild);
		delete m_pFirstChild;
	}
	if (m_pParent)
		m_pParent->deleteChild(this);

	assert(m_pFirstChild == 0);
	assert(m_pLastChild == 0);
	assert(m_numChildren == 0);

	EnableIRQs();
}


wsWindow::wsWindow(
		wsWindow *pParent,
		u16 id,
		s32 xs,
		s32 ys,
		s32 xe,
		s32 ye,
		u32 style) :
	m_id(id),
	m_style(style),
	m_rect(xs,ys,xe,ye)
	// m_rect_abs(xs,ys,xe,ye),
	// m_rect_client(xs,ys,xe,ye)
{
	// LOG("wsWindow(0x%08x,%d, %d,%d,%d,%d, 0x%08x)",(u32) this, id,xs,ys,xe,ye,style);
	
	m_pDC = 0;
	m_state = WIN_STATE_VISIBLE | WIN_STATE_UPDATE | WIN_STATE_DRAW;
	m_pFont = 0;
	
	m_pParent = pParent;
	m_pPrevSibling = 0;
	m_pNextSibling = 0;
	m_pFirstChild  = 0;
	m_pLastChild   = 0;
	m_numChildren  = 0;

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

	m_drag_constraint =
		DRAG_CONSTRAINT_OBJECT |
		DRAG_CONSTRAINT_SHOW;
}



void wsWindow::onSize()
{
	DBG_UPDATE(1,"onSize(%08x) m_rect(%d,%d,%d,%d)\n",(u32)this,m_rect.xs,m_rect.ys,m_rect.xe,m_rect.ye);

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
		DBG_UPDATE(2,"parent_rect(%d,%d,%d,%d)\n",parent_rect.xs,parent_rect.ys,parent_rect.xe,parent_rect.ye);

		m_rect_abs.makeRelative(parent_rect);
		m_clip_abs.assign(m_rect_abs);
		m_clip_abs.intersect(parent_rect);

		m_rect_client.makeRelative(parent_rect);
		m_clip_client.assign(m_rect_client);
		m_clip_client.intersect(parent_rect);
	}
	
	DBG_UPDATE(2,"m_rect_abs(%d,%d,%d,%d)\n",m_rect_abs.xs,m_rect_abs.ys,m_rect_abs.xe,m_rect_abs.ye);
	DBG_UPDATE(2,"m_rect_client(%d,%d,%d,%d)\n",m_rect_client.xs,m_rect_client.ys,m_rect_client.xe,m_rect_client.ye);
	DBG_UPDATE(2,"m_clip_abs(%d,%d,%d,%d)\n",m_clip_abs.xs,m_clip_abs.ys,m_clip_abs.xe,m_clip_abs.ye);
	DBG_UPDATE(2,"m_clip_client(%d,%d,%d,%d)\n",m_clip_client.xs,m_clip_client.ys,m_clip_client.xe,m_clip_client.ye);
	
}



wsApplication *wsWindow::getApplication() const
{
	const wsWindow *p = this;
	while (p && !(p->m_style & WIN_STYLE_APPLICATION))
		p = p->m_pParent;
	if (!p)
	{
		LOG_ERROR("win(%08x) could not find application!",(u32)this);
	}
	return (wsApplication *) p;
}



wsTopLevelWindow *wsWindow::getTopWindow() const
{
	const wsWindow *p = this->m_pParent;
	while (p && !(p->m_style & WIN_STYLE_TOP_LEVEL))
		p = p->m_pParent;
	if (!p)
	{
		LOG_ERROR("win(%08x) could not find top level window!",(u32)this);
	}
	return (wsTopLevelWindow *) p;
}


void wsWindow::addChild(wsWindow *pWin)
{
	// LOG("win(%X)::addChild(%X) numChildren=%d",(u32)this, (u32) pWin, m_numChildren);
	
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


void wsWindow::deleteChild(wsWindow *pWin)
{
	// LOG("win(%X)::deleteChild(%X) numChildren=%d",(u32)this, (u32) pWin, m_numChildren);
	
	assert(pWin);
	assert(pWin->m_pParent == this);
	
	if (pWin->m_pNextSibling)
		pWin->m_pNextSibling->m_pPrevSibling = pWin->m_pPrevSibling;
	if (pWin->m_pPrevSibling)
		pWin->m_pPrevSibling->m_pNextSibling = pWin->m_pNextSibling;
	if (pWin == m_pFirstChild)
		m_pFirstChild = pWin->m_pNextSibling;
	if (pWin == m_pLastChild)
		m_pLastChild = pWin->m_pPrevSibling;
	
	m_numChildren--;
	
	m_pDC->invalidate(pWin->m_rect_abs);	
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




void wsWindow::onDraw()
{
	#if 0
		delay(1);
		printf("draw(%08x:%d) m_state(%08x) ",
			(u32)this,
			m_id,
			m_state);
		if (m_state & WIN_STATE_DRAW)
			printf("DRAW ");
		if (m_state & WIN_STATE_REDRAW)
			printf("REDRAW ");
		if (m_state & WIN_STATE_INVALID)
		{
			printf("INVALID(%d,%d,%d,%d) ",
				m_pDC->getInvalid().xs,
				m_pDC->getInvalid().ys,
				m_pDC->getInvalid().xe,
				m_pDC->getInvalid().ye);
		}
		printf("\n");
	#endif
	
	// draw self
	#ifdef DEBUG_UPDATE
		if (!m_pDC->getInvalid().isEmpty())
			DBG_UPDATE(1,"draw(%08x:%d) invalid(%d,%d,%d,%d)\n",(u32)this,m_id,
				m_pDC->getInvalid().xs,
				m_pDC->getInvalid().ys,
				m_pDC->getInvalid().xe,
				m_pDC->getInvalid().ye);
	#endif
	
	if (m_style & WIN_STYLE_3D)
	{
		DBG_UPDATE(1,"draw(%08x:%d) 2d frame(%d,%d,%d,%d)\n",(u32)this,m_id,m_rect.xs,m_rect.ys,m_rect.xe,m_rect.ye);
		DBG_UPDATE(2,"                 clip(%d,%d,%d,%d)\n",m_clip_abs.xs,m_clip_abs.ys,m_clip_abs.xe,m_clip_abs.ye);

		m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);
		m_pDC->draw3DFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			windowFrameColors );
	}
	else if (m_style & WIN_STYLE_2D)
	{
		DBG_UPDATE(1,"draw(%08x:%d) 2d frame(%d,%d,%d,%d)\n",(u32)this,m_id,m_rect.xs,m_rect.ys,m_rect.xe,m_rect.ye);
		DBG_UPDATE(2,"                 clip(%d,%d,%d,%d)\n",m_clip_abs.xs,m_clip_abs.ys,m_clip_abs.xe,m_clip_abs.ye);

		m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);
		m_pDC->drawFrame(
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			m_fore_color );
	}
	
	if (!(m_style & WIN_STYLE_TRANSPARENT))
	{
		DBG_UPDATE(2,"draw(%08x) fill(%d,%d,%d,%d)\n",(u32)this,m_rect_client.xs,m_rect_client.ys,m_rect_client.xe,m_rect_client.ye);
		DBG_UPDATE(2,"           clip(%d,%d,%d,%d)\n",m_clip_client.xs,m_clip_client.ys,m_clip_client.xe,m_clip_client.ye);

		m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
		m_pDC->fillFrame(
			m_rect_client.xs,
			m_rect_client.ys,
			m_rect_client.xe,
			m_rect_client.ye,
			m_back_color);
	}
}


void wsWindow::setText(const char *text)
	// client level only!
	// sets the invalid region!
{
	m_text = text;
	m_pDC->invalidate(m_rect_abs);
	// setBit(m_state,WIN_STATE_DRAW);
}


void wsWindow::setText(const CString &text)
{
	setText((const char *)text);
}


void wsWindow::resize(s32 xs, s32 ys, s32 xe, s32 ye )
	// currently only resizes this window
	// will need to resize/move children as well
	// which, in turn, requires more care with
	// the window rectangles.
{
	m_pDC->invalidate(m_rect_abs);
	m_rect.assign(xs,ys,xe,ye);
	setBit(m_state,WIN_STATE_UPDATE);
}

void wsWindow::move( s32 x, s32 y )
{
	m_pDC->invalidate(m_rect_abs);
	s32 w = m_rect.xe - m_rect.xs + 1;
	s32 h = m_rect.ye - m_rect.ys + 1;
	m_rect.assign(x,y,x+w-1,y+h-1);
	setBit(m_state,WIN_STATE_UPDATE);
		
}



void wsWindow::show()
{
	if (!(m_state & WIN_STATE_VISIBLE))
	{
		setBit(m_state,WIN_STATE_VISIBLE | WIN_STATE_DRAW);
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
		clearBit(m_state,WIN_STATE_VISIBLE);
	}		
}



wsWindow* wsWindow::hitTest(s32 x, s32 y)
{
	if ((m_style & WIN_STYLE_TOUCH) &&
		(m_state & WIN_STATE_VISIBLE) &&
		(m_state & WIN_STATE_PARENT_VISIBLE) &&
		m_clip_abs.intersects(x,y))
	{
		setBit(m_state,WIN_STATE_IS_TOUCHED);
		onUpdateTouch(1);
		#if DEBUG_TOUCH
			LOG("wsWindow::hitTest(%d,%d)=%08x:%d",x,y,(u32)this,m_id);
		#endif
		if (m_style & WIN_STYLE_DRAG)
			onUpdateDragBegin();
		return this;
	}
	for (wsWindow *p = m_pFirstChild; p; p=p->m_pNextSibling)
	{
		wsWindow *found = p->hitTest(x,y);
		if (found)
		{
			#if 0
				LOG("wsWindow::hitTest(%d,%d) returning %08x:%d",x,y,(u32)found,found->m_id);
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
					onUpdateDragBegin();
				}
				else 	// regular button lost focus
				{
					clearBit(m_state,WIN_STATE_IS_TOUCHED);
					onUpdateTouch(0);
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
			onUpdateTouch(0);
			
			if (m_state & WIN_STATE_DRAGGING)
			{
				clearBit(m_state,WIN_STATE_DRAGGING);
				onUpdateDragEnd();
			}
			else if (m_style & WIN_STYLE_CLICK)
			{
				if (!touch_state->event_sent)
					onUpdateClick();
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
				onUpdateClick();
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
				onUpdateDragBegin();
			}
		}
		else if (touch_state->x != touch_state->last_x ||
				 touch_state->y != touch_state->last_y)
		{
			touch_state->last_x = touch_state->x;
			touch_state->last_y = touch_state->y;
			onUpdateDragMove();
		}

		return;
	}

	
	#define TOUCH_LONG_INTERVAL  800
	
	if (m_style & WIN_STYLE_CLICK_LONG &&
		!touch_state->event_sent &&
		now > touch_state->time + TOUCH_LONG_INTERVAL)
	{
		touch_state->event_sent = true;
		onUpdateLongClick();
		return;
	}

}


//-----------------------------------------
// base class state handlers
//-----------------------------------------
// These are not event handlers!
//
// They merely change the state of the object, possibly
// enqueue an event, and return quickly.

void wsWindow::onUpdateTouch(bool touched)
	// called before validation, sets the DRAW bit directly
{
	setBit(m_state,WIN_STATE_DRAW);
}


void wsWindow::onUpdateClick()
{
	getApplication()->addEvent(new wsEvent(
		EVT_TYPE_WINDOW,
		EVENT_CLICK,
		this ));
}

void wsWindow::onUpdateLongClick()
{
	getApplication()->addEvent(new wsEvent(
		EVT_TYPE_WINDOW,
		EVENT_LONG_CLICK,
		this ));
}

void wsWindow::onUpdateDblClick()
{}



//---------------------------------
// Generic dragging behavior
//---------------------------------

#define  DEBUG_DRAG  0
#define  DRAG_OUT_MARGIN  6
	// number of pixels that will show on screen
	// if they drag out right or bottom

void wsWindow::onUpdateDragBegin()
{
	wsApplication *app = getApplication();
	touchState_t *touch_state = app->getTouchState();
	
	// the drag could begin due to timing, or
	// the mouse being dragged off the object
	// in either case, we start by corraling
	// the x,y to within the object

	s32 x = touch_state->x;
	s32 y = touch_state->y;
	
	if (m_drag_constraint & DRAG_CONSTRAINT_OBJECT)
	{
		if (x < m_rect_abs.xs) x = m_rect_abs.xs;
		if (y < m_rect_abs.ys) y = m_rect_abs.ys;
		if (x > m_rect_abs.xe) x = m_rect_abs.xe;
		if (y > m_rect_abs.ye) y = m_rect_abs.ye;
	}
	
	// make it relative to the object
	// and remember it

	touch_state->drag_x  = x - m_rect_abs.xs;
	touch_state->drag_y  = y - m_rect_abs.ys;

	#if DEBUG_DRAG
		LOG("onUpdateDragBegin(%i,%i) rect(%d,%d,%d,%d) drag_x=%d drag_y=%d",
			touch_state->x,
			touch_state->y,
			m_rect_abs.xs,
			m_rect_abs.ys,
			m_rect_abs.xe,
			m_rect_abs.ye,
			touch_state->drag_x,
			touch_state->drag_y);
		LOG("constraint=%02x",m_drag_constraint);
	#endif
}


void wsWindow::onUpdateDragMove()
{
	wsApplication *app = getApplication();
	touchState_t *touch_state = app->getTouchState();
	wsRect client_rect(m_pParent->getClientRect());

	#if DEBUG_DRAG
		print_rect("client_rect",&client_rect);
	#endif

	s32 x = touch_state->x;
	s32 y = touch_state->y;
	s32 drag_x = touch_state->drag_x;
	s32 drag_y = touch_state->drag_y;
	s32 width = m_rect.getWidth();
	s32 height = m_rect.getHeight();
	
	// offset the abs coordinates to the
	// upper left corner of the object
	// constrained by absolute 0,0

	x -= drag_x;
	y -= drag_y;
	#if DEBUG_DRAG
		LOG("  upper left=%d,%d",x,y);
	#endif

	// make sure it fits in the client area if requested
	
	if (m_drag_constraint & DRAG_CONSTRAINT_FIT)
	{
		if (x + width-1 > client_rect.xe) x = client_rect.xe - width + 1;
		if (y + height-1 > client_rect.ye) y = client_rect.ye - height + 1;
		if (x < client_rect.xs) x = client_rect.xs;
		if (y < client_rect.ys) y = client_rect.ys;
	}
	else if (m_drag_constraint & DRAG_CONSTRAINT_SHOW)
	{
		if (x > client_rect.xe - DRAG_OUT_MARGIN) x = client_rect.xe - DRAG_OUT_MARGIN;
		if (y > client_rect.ye - DRAG_OUT_MARGIN) y = client_rect.ye - DRAG_OUT_MARGIN;
		if (x + width - 1 < client_rect.xs + DRAG_OUT_MARGIN) x = client_rect.xs + DRAG_OUT_MARGIN - width + 1;
		if (y + height - 1 < client_rect.ys + DRAG_OUT_MARGIN) y = client_rect.ys + DRAG_OUT_MARGIN - height + 1;
	}
	
	if (m_drag_constraint & DRAG_CONSTRAINT_X)
	{
		y = m_rect_abs.ys;
	}
	if (m_drag_constraint & DRAG_CONSTRAINT_Y)
	{
		x = m_rect_abs.xs;
	}

	#if DEBUG_DRAG
		LOG("  constrained=%d,%d",x,y);
	#endif
	
	// make it relative to the parent client area
	// and move it
	
	x -= client_rect.xs;
	y -= client_rect.ys;
	
	#if DEBUG_DRAG
		LOG("     relative=%d,%d",x,y);
	#endif
	
	move(x,y);
}


void wsWindow::onUpdateDragEnd()
{
	#if DEBUG_DRAG
		wsApplication *app = getApplication();
		touchState_t *touch_state = app->getTouchState();
		LOG("onUpdateDragEnd(%d,%d)",
			touch_state->x,
			touch_state->y);
	#endif
}
	
	




void wsWindow::update()
{
	// inherit bits from parent
	
	if (m_pParent)
	{
		DBG_UPDATE(0,"update(%08x:%d) start m_state(%08x) inheriting(%08x)\n",
				   (u32)this,m_id,
				   m_state,
				   m_pParent->m_state);
		INC_UPDATE_LEVEL();

		// we inherit the update, draw and redraw bits directly,
		// but we set out own parent visible based on the parent's
		// state.
		
		setBit(m_state,m_pParent->m_state & (
			WIN_STATE_UPDATE |
			WIN_STATE_DRAW |
			WIN_STATE_REDRAW ));
		clearBit(m_state,WIN_STATE_PARENT_VISIBLE);
		
		if ((m_pParent->m_state & WIN_STATE_VISIBLE) &&
			(m_pParent->m_state & WIN_STATE_PARENT_VISIBLE))
			setBit(m_state,WIN_STATE_PARENT_VISIBLE);

		DBG_UPDATE(1,"inherited m_state(%08x)\n",m_state);
	}

	// resize the window if needed
	
	if (m_state & WIN_STATE_UPDATE)
	{
		DBG_UPDATE(1,"calling onSize()\n",0);
		setBit(m_state,WIN_STATE_DRAW);
		onSize();
	}
	
	// draw the window if neeeded
	
	if ((m_state & WIN_STATE_PARENT_VISIBLE) &&
		(m_state & WIN_STATE_VISIBLE))
	{
		const wsRect &invalid = m_pDC->getInvalid();
		if (!(m_state & (WIN_STATE_DRAW | WIN_STATE_REDRAW)) &&
		    m_rect_abs.intersects(invalid))
			setBit(m_state,WIN_STATE_INVALID);
			
		if (m_state & (WIN_STATE_DRAW | WIN_STATE_REDRAW | WIN_STATE_INVALID))
		{
			DBG_UPDATE(1,"calling onDraw()\n",0);
			onDraw();
		}
	}

	// update children with inherited state bits

	for (wsWindow *p = m_pFirstChild; p; p=p->m_pNextSibling)
	{
		p->update();
	}
	
	// clear our handled state bits
	
	clearBit(m_state,
		WIN_STATE_UPDATE |
		WIN_STATE_DRAW |
		WIN_STATE_REDRAW |
		WIN_STATE_INVALID);
	
	#ifdef DEBUG_UPDATE
		if (m_pParent)
		{
			DEC_UPDATE_LEVEL();
			DBG_UPDATE(0,"end update(%08x:%d)  m_state(%08x)\n",(u32)this,m_id,m_state);
		}
	#endif	
}


// virtual

void wsWindow::updateFrame()
	// an update call tree that is 10 times per second
{
	for (wsWindow *p = m_pFirstChild; p; p=p->m_pNextSibling)
	{
		p->updateFrame();
	}
}
