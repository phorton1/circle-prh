//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.
// (c) Copyright 2019 Patrick Horton- no rights reserved, 

#ifndef _wsWindow_h
#define _wsWindow_h

#include "wsDC.h"
#include "wsTheme.h"

// #define DEBUG_UPDATE

#ifdef DEBUG_UPDATE
	extern int debug_update;
	extern void debugUpdate(int num);
#else
	#define debugUpdate(num)
#endif


//------------------------------------
// wsEventHandler
//------------------------------------

class wsEventHandler 
{
	public:
		
		wsEventHandler() {}
		~wsEventHandler() {}
		
		virtual u32 handleEvent(wsEvent *event) { return 0; };
			// at this time the semantics of the return value are unclear
			// I was thinking it could be the object, if any, that handled
			// the event, but it would probably be better to have explicit
			// bits, like STOP_PROPOGATION, ALLOW, etc.
	
	private:
		
};	// wsEventHandler



//------------------------------------
// wsWindow
//------------------------------------
// some states are mutually exclusive.
// - there should be only one WIN_STYLE_APPLICATION
// - only top level windows should set WIN_STYLE_TOP_LEVEL
// - WIN_STYLE_3D has precedence over WIN_STYLE_2D
// - CLICK_REPEAT has precedence over CLICK_LONG and CLICK_DRAG
// - CLICK_DRAG has precedence over CLICK_LONG


#define WIN_STYLE_APPLICATION       0x00000001
#define WIN_STYLE_TOP_LEVEL         0x00000002
#define WIN_STYLE_POPUP          	0x00000004

#define WIN_STYLE_2D                0x00000010
#define WIN_STYLE_3D                0x00000020
#define WIN_STYLE_TRANSPARENT    	0x00000040

#define WIN_STYLE_TOUCH   			0x00000100
#define WIN_STYLE_CLICK   			0x00000200
#define WIN_STYLE_CLICK_DBL			0x00001000
#define WIN_STYLE_CLICK_LONG		0x00002000
#define WIN_STYLE_CLICK_REPEAT		0x00004000
#define WIN_STYLE_DRAG				0x00008000

#define WIN_STYLE_TOUCH_TIMING_EVENTS	( \
	WIN_STYLE_DRAG				| \
    WIN_STYLE_CLICK_DBL			| \
    WIN_STYLE_CLICK_LONG		| \
    WIN_STYLE_CLICK_REPEAT      )


// #define WIN_STYLE_OWNER_DRAW			0x02000000
// #define WIN_STYLE_MOUSE_OVER			0x04000000
// #define WIN_STYLE_LONG_TOUCH   		0x08000000
// #define WIN_STYLE_DRAG   			0x00100000
// #define WIN_STYLE_ZOOM   			0x00200000
// #define WIN_STYLE_FLING   			0x00400000
// #define WIN_STYLE_TRANSPARENT    	0x00600000


#define WIN_STATE_VISIBLE           0x00000001

#define WIN_STATE_PARENT_VISIBLE    0x00000010
#define WIN_STATE_UPDATE            0x00000020
#define WIN_STATE_DRAW              0x00000040
#define WIN_STATE_REDRAW            0x00000080
#define WIN_STATE_INVALID           0x00000100

#define WIN_STATE_IS_TOUCHED		0x00001000
#define WIN_STATE_TOUCH_CHANGED		0x00002000
#define WIN_STATE_DRAGGING			0x00004000

// How it works.
//
//     UPDATE ==> DRAW ==> REDRAW
//     UPDATE ==> onSize()
//          set m_rect_abs
//          set m_rect_client
//          set m_clip_abs
//          set m_clip_client
//     PARENT_VISIBLE = parent->VISIBLE & parent->PARENT_VISIBLE
//     VISIBLE =>
//     		!DRAW && !REDRAW && intersects(invalid) => INVALID
//          DRAW | REDRAW | INVALID => onDraw()
//  		onDraw() => setClip(m_clip_xxx,INVALID);
//          setClip(INVALID) ==> clip.intersect(invalid)
//
// STATE_UPDATE means that the parent coordinates (may have)
//    changed and so the object's absolute and clipping coordinates
//    need to be recalculated.  This implies that the whole objecct
//    needs to be redrawn.
// STATE_DRAW means that the whole object needs to be drawn, perhaps
//    as a result of it intersecting with the invalid screen
//    region, or because it is newly being shown. So the frame,
//    background, and contents must all be redrawn.
// STATE_REDRAW means that only the semantic value of the object has
//    changed, and thus the object is free to use an optimized
//    redrawing method making assumptions about what is currently
//    visible.  This is important for highly dynamic controls
//    like sliders, scrollbars, vu meters, and so on.
// STATE_INVALID means that there is an invalidated region,
//    the object would not otherwise redraw,
//    and the object intersects with the invalidated region.
//
// PARENT_VISIBLE is recursively set during the update() tree
//    so objects know that if their parent is not visible, they
//    should not be drawn (or accept hit tests), while yet allowing
//    each object to maintain it's own visibility state.

	
// #define WIN_STATE_ENABLE            	0x00000002
// #define WIN_STATE_UPDATE            	0x00000004
// #define WIN_STATE_MOUSE_OVER        	0x00000010
// #define WIN_STATE_HAS_FOCUS         	0x00000020
// #define WIN_STATE_HAS_TOUCH_FOCUS   	0x00000040
// #define WIN_STATE_IS_DRAGGING		0x00000200
// #define WIN_STATE_IS_ZOOMING			0x00000400
// #define WIN_STATE_IS_FLINGING		0x00000800


#define DRAG_CONSTRAINT_X			0x01
#define DRAG_CONSTRAINT_Y			0x02
#define DRAG_CONSTRAINT_FIT			0x04
#define DRAG_CONSTRAINT_SHOW		0x08
#define DRAG_CONSTRAINT_OBJECT		0x10


class wsWindow : public wsEventHandler
{
	public:
	
		wsWindow(wsWindow *pParent, u16 id, s32 xs, s32 ys, s32 xe, s32 ye, u32 style=0);
		~wsWindow() {}	

		u16 getID() const			{ return m_id; }
		u32 getStyle() const	   	{ return m_style; };
		u32 getState() const		{ return m_state; }
		void setStyle(u32 style)	{ m_style = style; };
		
		wsColor getForeColor() const	{ return m_fore_color; }
		wsColor getBackColor() const	{ return m_back_color; }
		wsAlignType getAlign() const 	{ return m_align; }
		
		const wsFont *getFont()				{ return m_pFont; }
		void setFont(const wsFont *pFont) 	{ m_pFont = pFont; }
		void setForeColor(wsColor color) 	{ m_fore_color = color; }
		void setBackColor(wsColor color) 	{ m_back_color = color; }
		void setAlign(wsAlignType align)	{ m_align = align; }
		
		void show();
		void hide();
		void resize(s32 xs, s32 ys, s32 xe, s32 ye );
		void move( s32 x, s32 y );
		
		const wsRect &getRect() const				{ return m_rect; }			
		const wsRect &getOuterRect() const 			{ return m_rect_abs; }		
		const wsRect &getClientRect() const 		{ return m_rect_client; }
		
		const s32 getWidth() const  { return m_rect.getWidth(); }
		const s32 getHeight() const { return m_rect.getHeight(); }
		const s32 getClientWidth() const  { return m_rect_client.getWidth(); }
		const s32 getClientHeight() const { return m_rect_client.getHeight(); }
		
		const char *getText()				{ return (const char *) m_text; }
		const CString &getString()			{ return (const CString &) m_text; }
		void  setText(const char *text);
		void  setText(const CString &text);
		
		wsWindow *getParent()   	{ return m_pParent;       }
		wsWindow *getPrevSibling()  { return m_pPrevSibling;  }
		wsWindow *getNextSibling()  { return m_pNextSibling;  }
		wsWindow *getFirstChild()   { return m_pFirstChild;   }
		wsWindow *getLastChild()    { return m_pLastChild;    }
		
		void addChild(wsWindow *pWin);
		void deleteChild(wsWindow *pWin);
		u16 getNumChildren()		{ return m_numChildren; }

		virtual wsApplication *getApplication() const;
		wsWindow *findChildByID(u16 id);

		// void setVirtualSize(s32 width, s32 height);
		// void setVirtualOffset(s32 xoff, s32 yoff);
		// const wsRect &getVirtualRect() const  		{ return m_rect_virt;  }  
		// const wsRect &getVirtualVisibleRect() const { return m_rect_vis;  }
		// wsRect m_rect_virt;		// visible virtual screen size (0..any number) (not limited to screen size)
		// wsRect m_rect_vis;		// visible virtual screen rectangle (clipped to window size)
		
		void setDragConstraint(u8 constraint)  		{ m_drag_constraint = constraint; }
		u8  getDragConstraint() const 				{ return m_drag_constraint; }
		
	protected:
		
		friend class wsApplication;
		friend class wsTopLevelWindow;
		
		wsDC *getDC() const		{ return m_pDC; }
		void setDC(wsDC *pDC)	{ m_pDC = pDC; }

		virtual void update();
		virtual void onDraw();
		virtual void onSize();
		virtual wsWindow *hitTest(s32 x, s32 y);
		
		void updateTouch(touchState_t *touch_state);
			// called directly by mouse and touch handlers
		virtual void onUpdateTouch(bool touched);
			// called directly from updateTouch, must NOT
			// call setInvalidate()!

		// derived implementations of the following methods must take care to
		// call setInvalidate() if they want to update, draw, or redraw
		// the object, as they are called from update() after m_pDC->validate(),
		// and thus must expand the invalid region to include the object.
		
		virtual void onUpdateClick();
		virtual void onUpdateDblClick();
		virtual void onUpdateLongClick();
		virtual void onUpdateDragBegin();
		virtual void onUpdateDragMove();
		virtual void onUpdateDragEnd();

		u16 m_id;
		u32 m_style;
		u32 m_state;

		wsRect m_rect;			// relative to parent (constrution coordinates, limited to screen size)
		wsRect m_rect_abs;		// absolute outer screen coordinates
		wsRect m_rect_client;   // absolute inner (client) screen coordinates
		wsRect m_clip_abs;   	// the client rect clipped by the parent client area
		wsRect m_clip_client;   // the abs rect clipped by the parent client area
		
		u16 m_numChildren;
		wsWindow *m_pParent;
		wsWindow *m_pPrevSibling;
		wsWindow *m_pNextSibling;
		wsWindow *m_pFirstChild;
		wsWindow *m_pLastChild;
		
		wsDC *m_pDC;
		const wsFont *m_pFont;
		wsColor m_fore_color;
		wsColor m_back_color;
		wsAlignType m_align;

		CString m_text;
		u8 m_drag_constraint;
};


#endif  // !_wsWindow_h
