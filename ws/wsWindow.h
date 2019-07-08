//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsWindow_h
#define _wsWindow_h

#include <circle/string.h>
#include <circle/screen.h>
#include <circle/input/mouse.h>
#include <circle/input/touchscreen.h>
#include "wsColor.h"
#include "wsFont.h"
#include "wsTheme.h"


// defines

#define ALIGN_H_LEFT                                  (1<<0)
#define ALIGN_H_CENTER                                (1<<1)
#define ALIGN_H_RIGHT                                 (1<<2)
#define ALIGN_V_TOP                                   (1<<3)
#define ALIGN_V_CENTER                                (1<<4)
#define ALIGN_V_BOTTOM                                (1<<5)
#define ALIGN_BOTTOM_RIGHT                            (ALIGN_V_BOTTOM|ALIGN_H_RIGHT)
#define ALIGN_BOTTOM_CENTER                           (ALIGN_V_BOTTOM|ALIGN_H_CENTER)
#define ALIGN_BOTTOM_LEFT                             (ALIGN_V_BOTTOM|ALIGN_H_LEFT)
#define ALIGN_CENTER_RIGHT                            (ALIGN_V_CENTER|ALIGN_H_RIGHT)
#define ALIGN_CENTER                                  (ALIGN_V_CENTER|ALIGN_H_CENTER)
#define ALIGN_CENTER_LEFT                             (ALIGN_V_CENTER|ALIGN_H_LEFT)
#define ALIGN_TOP_RIGHT                               (ALIGN_V_TOP|ALIGN_H_RIGHT)
#define ALIGN_TOP_CENTER                              (ALIGN_V_TOP|ALIGN_H_CENTER)
#define ALIGN_TOP_LEFT                                (ALIGN_V_TOP|ALIGN_H_LEFT)

// forwards

class wsRect;
class wsEvent;
class wsWindow;
class wsApplication;

// types

typedef u8 		wsAlignType;


//------------------------------------
// wsRect
//------------------------------------

extern void print_rect(const char *name, wsRect *rect);

class wsRect
{
public:
	
	wsRect()
	{
		xs = 0;
		ys = 0;
		xe = 0;
		ye = 0;
	}

	wsRect(u16 x0, u16 y0, u16 x1, u16 y1)
	{
		xs = x0;
		ys = y0;
		xe = x1;
		ye = y1;
	}
	
	wsRect(const wsRect *pRect)
	{
		xs = pRect->xs;
		ys = pRect->ys;
		xe = pRect->xe;
		ye = pRect->ye;
	}
	
	void makeRelative(const wsRect *pRect)
	{
		xs += pRect->xs;
		ys += pRect->ys;
		xe += pRect->xs;
		ye += pRect->ys;
	}
	
	u16 getWidth() const { return xe-xs+1; }
	u16 getHeight() const { return ye-ys+1; }
	void assign(u16 x0, u16 y0, u16 x1, u16 y1)
	{
		xs = x0;
		ys = y0;
		xe = x1;
		ye = y1;
	}

	u16 xs;
	u16 ys;
	u16 xe;
	u16 ye;

};



//-----------------------------------
// wsDeviceContext
//-----------------------------------

#define NUM_OPT_DRIVERS  			3
#define OPT_DRIVER_DRAW_LINE        0
#define OPT_DRIVER_FILL_FRAME       1
#define OPT_DRIVER_FILL_AREA        2


class wsDC
{
	public:
	
		~wsDC();

		wsDC(
			CScreenDeviceBase *pScreen) :
			m_pScreen(pScreen),
			m_xdim(m_pScreen->GetWidth()),
			m_ydim(m_pScreen->GetHeight())
			{
				m_pFont = 0;
				m_fore_color = 0;
				m_back_color = 0;
				m_align = 0;
				m_hspace = 0;
				m_vspace = 0;
				for (int i=0; i<NUM_OPT_DRIVERS; i++)
					m_opt_driver[i] = 0;
			}

		CScreenDeviceBase *getScreen() 	{ return m_pScreen; }
		u16 getXDim( void ) const { return m_xdim; }
		u16 getYDim( void ) const { return m_ydim; }
		void setPixel(u16 x, u16 y, wsColor color)
			{ m_pScreen->SetPixel(x,y,color); }

		void fillScreen( wsColor color );
		void fillFrame( u16 x0, u16 y0, u16 x1, u16 y1, wsColor color );
		void drawLine( u16 x0, u16 y0, u16 x1, u16 y1, wsColor color );
		void drawFrame( u16 x0, u16 y0, u16 x1, u16 y1, wsColor color );
		void draw3DFrame( u16 xs, u16 ys, u16 xe, u16 ye, const wsColor *pColor );

		void fillRoundFrame( u16 x0, u16 y0, u16 x1, u16 y1, u16 r, wsColor color );
		void drawMesh( u16 x0, u16 y0, u16 x1, u16 y1, wsColor color );
		void drawRoundFrame( u16 x0, u16 y0, u16 x1, u16 y1, u16 r, wsColor color );
		void drawCircle( u16 x, u16 y, u16 r, wsColor color );
		void fillCircle( u16 x, u16 y, u16 r, wsColor color );
		void drawArc( u16 x, u16 y, u16 r, u8 s, wsColor color );

		void setFont( const wsFont *pFont )  { m_pFont = pFont; }
		void putChar( char chr, u16 x, u16 y, wsColor fc, wsColor bc );
		void putString( u16 x, u16 y, const char* str );
		
		void putText(
			wsColor bc,
			wsColor fc,
			const wsRect *area,
			wsAlignType align,
			s16     hspace,
			s16     vspace,
			const char *text);

		void setForeColor( wsColor color )		{ m_fore_color = color; }
		void setBackColor( wsColor color )		{ m_back_color = color; }
		void setFontHSpace( s16 space )			{ m_hspace = space; }
		void setFontVSpace( s16 space ) 		{ m_vspace = space; }
		
		void consolePutString( char* str );
		void consoleSetArea( u16 xs, u16 ys, u16 xe, u16 ye );
		void consoleSetForecolor( wsColor color );
		void consoleSetBackcolor( wsColor color );
		
		void driverRegister(void *pUnusedScreen, u8 type, void *driver);
		static void driverRegisterStub(void *pThis, void *pUnusedScreen, u8 type, void *driver);
			// we don't need the pointer to the screen device inasmuch
			// as we already have it.  UGUI requires it because it is "C" code
			// calling static methods, divorced from the C++ object rst wrote.

	private:

		wsDC() {}
		
		CScreenDeviceBase *m_pScreen;
		
		u16 m_xdim;
		u16 m_ydim;

		const wsFont *m_pFont;
		wsColor m_fore_color;
		wsColor m_back_color;
		wsAlignType m_align;
		s16 m_hspace;
		s16 m_vspace;
		
		void *m_opt_driver[NUM_OPT_DRIVERS];
		
};	// wsDeviceContext




//------------------------------------
// wsEventHandler
//------------------------------------

class wsEventHandler 
{
	public:
		
		wsEventHandler() {}
		~wsEventHandler() {}
		
		virtual u32 handleEvent(wsEvent *event) { return 0; };
	
	private:
		
};	// wsEventHandler



//------------------------------------
// wsWindow
//------------------------------------

#define WIN_STYLE_APPLICATION       0x80000000
#define WIN_STYLE_TOP_LEVEL         0x40000000

#define WIN_STYLE_2D                0x20000000
#define WIN_STYLE_3D                0x10000000
#define WIN_STYLE_TRANSPARENT    	0x08000000

#define WIN_STYLE_TOUCH   			0x04000000

// #define WIN_STYLE_POPUP          	0x01000000
// #define WIN_STYLE_OWNER_DRAW			0x02000000
// #define WIN_STYLE_MOUSE_OVER			0x04000000
// #define WIN_STYLE_LONG_TOUCH   		0x08000000
// #define WIN_STYLE_DRAG   			0x00100000
// #define WIN_STYLE_ZOOM   			0x00200000
// #define WIN_STYLE_FLING   			0x00400000
// #define WIN_STYLE_TRANSPARENT    	0x00600000


#define WIN_STATE_VISIBLE           0x00000001
#define WIN_STATE_REDRAW            0x00000002
#define WIN_STATE_IS_TOUCHED		0x00000004
#define WIN_STATE_TOUCH_CHANGED		0x00000008

// #define WIN_STATE_ENABLE            	0x00000002
// #define WIN_STATE_UPDATE            	0x00000004
// #define WIN_STATE_MOUSE_OVER        	0x00000010
// #define WIN_STATE_HAS_FOCUS         	0x00000020
// #define WIN_STATE_HAS_TOUCH_FOCUS   	0x00000040
// #define WIN_STATE_IS_DRAGGING		0x00000200
// #define WIN_STATE_IS_ZOOMING			0x00000400
// #define WIN_STATE_IS_FLINGING		0x00000800


class wsWindow : public wsEventHandler
{
	public:
	
		wsWindow(wsWindow *pParent, u16 id, u16 xs, u16 ys, u16 xe, u16 ye, u32 style=0);
		~wsWindow() {}	

		u16 getID() const			{ return m_id; }
		u32 getStyle() const	   	{ return m_style; };
		u32 getState() const		{ return m_state; }
		
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
		
		void resize(u16 xs, u16 ys, u16 xe, u16 ye );
		void move( u16 x, u16 y );
		void setVirtualSize(u16 width, u16 height);
		void setVirtualOffset(u16 xoff, u16 yoff);
		
		const wsRect *getRect() const				{ return &m_rect; }			
		const wsRect *getOuterRect() const 			{ return &m_rect_abs; }		
		const wsRect *getClentRect() const 			{ return &m_rect_client; }	
		const wsRect *getVirtualRect() const  		{ return &m_rect_virt;  }  
		const wsRect *getVirtualVisibleRect() const { return &m_rect_vis;  }
		
		const u16 getWidth() const { return m_rect.getWidth(); }
		const u16 getHeight() const { return m_rect.getHeight(); }
		
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

		wsApplication *getApplication();
		wsWindow *findChildByID(u16 id);
		
	protected:
		
		friend class wsApplication;
		friend class wsTopLevelWindow;
		
		// void setStyle(u32 style)	{ m_style = style; };
		
		wsDC *getDC() const		{ return m_pDC; }
		void setDC(wsDC *pDC)	{ m_pDC = pDC; }

		virtual void update(bool visible);
		virtual void draw();
		
		virtual wsWindow *hitTest(unsigned x, unsigned y);

		u16 m_id;
		u32 m_style;
		u32 m_state;

		wsRect m_rect;			// relative to parent (constrution coordinates, limited to screen size)
		wsRect m_rect_abs;		// absolute outer screen coordinates
		wsRect m_rect_client;   // absolute inner (client) screen coordinates
		wsRect m_rect_virt;		// visible virtual screen size (0..any number) (not limited to screen size)
		wsRect m_rect_vis;		// visible virtual screen rectangle (clipped to window size)
		
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
};


//-------------------------------------------
// top level windows
//-------------------------------------------



class wsTopLevelWindow : public wsWindow
{
	public:
	
		wsTopLevelWindow(wsApplication *pApp, u16 id, u16 xs, u16 ys, u16 xe, u16 ye, u32 style=0);
		~wsTopLevelWindow() {}
		
		wsApplication *getApplication() const { return (wsApplication *) m_pParent; }

		void show();
		void hide();
		
	protected:
		
		friend class wsApplication;
		
		virtual wsWindow *hitTest(unsigned x, unsigned y);
		
		wsTopLevelWindow *m_pPrevWindow;
		wsTopLevelWindow *m_pNextWindow;
		
};


//-------------------------------------------
// controls
//-------------------------------------------

class wsControl : public wsWindow
{
	public:
	
		~wsControl() {}
		
		wsControl(wsWindow *pParent, u16 id, u16 xs, u16 ys, u16 xe, u16 ye, u32 style=0) :
			wsWindow(pParent,id,xs,ys,xe,ye,style) {}
};



class wsStaticText : public wsControl
{
	public:
	
		~wsStaticText() {}
		
		wsStaticText(wsWindow *pParent, u16 id, const char *text, u16 xs, u16 ys, u16 xe, u16 ye) :
			wsControl(pParent,id,xs,ys,xe,ye)
		{
			if (text)
				setText(text);
		}
		
	protected:
	
		virtual void draw();
	
};




//---------------------------
// wsButton
//---------------------------

#define BTN_STATE_RELEASED                            0x0000
#define BTN_STATE_PRESSED                             0x0001

#define BTN_STYLE_2D                                  0x0001
#define BTN_STYLE_3D                                  0x0002
#define BTN_STYLE_TOGGLE_COLORS                       0x0004
#define BTN_STYLE_USE_ALTERNATE_COLORS                0x0008
#define BTN_STYLE_NO_FILL                             0x0010
#define BTN_STYLE_TOGGLE_VALUE                        0x0020

// #define BTN_STYLE_ALWAYS_REDRAW                    0x1000


class wsButton : public wsControl
{
	public:
	
		~wsButton() {}
		
		wsButton(wsWindow *pParent, u16 id, const char *text, u16 xs, u16 ys, u16 xe, u16 ye, u16 bstyle=0) :
			wsControl(pParent,id,xs,ys,xe,ye,
				WIN_STYLE_TOUCH | (
				(bstyle & BTN_STYLE_2D) ? WIN_STYLE_2D :
				(bstyle & BTN_STYLE_3D) ? WIN_STYLE_3D : 0))
		{
			m_button_state = 0;
			m_button_style = bstyle;
			m_fore_color = defaultButtonForeColor;
			m_back_color = defaultButtonReleasedColor;
			m_alt_back_color = defaultButtonPressedColor;
			m_alt_fore_color = m_fore_color;
			if (text)
				setText(text);
		}
 
		bool isPressed() const { return m_button_state & BTN_STATE_PRESSED; }
		void setAltBackColor(wsColor color)  {m_alt_back_color = color;}
		void setAltForeColor(wsColor color)  {m_alt_fore_color = color;}

	protected:
	
		u8 m_button_state;
		u16 m_button_style;
		wsColor m_alt_back_color;
		wsColor m_alt_fore_color;
		
		virtual void draw();
		virtual void update(bool visible);
		
};	// wsButton



//---------------------------
// wsCheckbox
//---------------------------

#define CHB_STATE_RELEASED                            0x0000
#define CHB_STATE_PRESSED                             0x0001
#define CHB_STATE_CHECKED                             0x0002

#define CHB_STYLE_2D                                  0x0001
#define CHB_STYLE_3D                                  0x0002
#define CHB_STYLE_TOGGLE_COLORS                       0x0004
#define CHB_STYLE_USE_ALTERNATE_COLORS                0x0008
#define CHB_STYLE_NO_FILL                             0x0010

// #define CHB_STYLE_ALWAYS_REDRAW                       0x0020


class wsCheckbox : public wsControl
{
	public:
	
		~wsCheckbox() {}
		wsCheckbox(wsWindow *pParent, u16 id, u8 checked, u16 xs, u16 ys, u16 xe, u16 ye, u16 cstyle=0) :
			wsControl(pParent,id,xs,ys,xe,ye,
  				WIN_STYLE_TOUCH | (
				(cstyle & CHB_STYLE_2D) ? WIN_STYLE_2D :
				(cstyle & CHB_STYLE_3D) ? WIN_STYLE_3D : 0))
		{
			m_checkbox_style = cstyle;
			m_checkbox_state = checked ? CHB_STATE_CHECKED : 0;
			m_align = ALIGN_TOP_LEFT;
			m_fore_color = defaultButtonForeColor;
			m_back_color = defaultButtonReleasedColor;
			m_alt_back_color = defaultButtonPressedColor;
			m_alt_fore_color = m_fore_color;
		}
		
		bool isChecked()  { return m_checkbox_state & CHB_STATE_CHECKED; }
		void setChecked(bool checked)
		{
			if (checked)
				m_checkbox_state |= CHB_STATE_CHECKED;
			else
				m_checkbox_state &= ~CHB_STATE_CHECKED;
			m_state |= WIN_STATE_REDRAW;
		}
		
		void setAltBackColor(wsColor color)  {m_alt_back_color = color;}
		void setAltForeColor(wsColor color)  {m_alt_fore_color = color;}

	protected:
	
		u8 m_checkbox_state;
		u16 m_checkbox_style;
		wsColor m_alt_back_color;
		wsColor m_alt_fore_color;
		
		virtual void draw();
		virtual void update(bool visible);

};	// wsCheckbox



//---------------------------
// wsImage
//---------------------------

class wsImage : public wsControl
{
	public:
	
		wsImage(wsWindow *pParent, u16 id, u8 value, u16 x, u16 y, u16 xe, u16 ye, u32 style=0);
		~wsImage() {}
};



//------------------------------------
// wsEvent 
//------------------------------------

#define EVT_TYPE_BUTTON    0x00000001
#define EVT_TYPE_CHECKBOX  0x00000002

#define BTN_EVENT_PRESSED    	 0x00000001
#define CHB_EVENT_VALUE_CHANGED  0x00000001

class wsEvent
{
	public:
	
		wsEvent(u32 type, u32 id, wsWindow *obj) :
			m_type(type),
			m_id(id),
			m_obj(obj)
		{
			m_pNext = 0;
			m_pPrev = 0;
		}

		~wsEvent() {}
		
		u32 getEventType() const		{ return m_type; }
		u32 getEventID() const			{ return m_id; }
		u16 getID() const 			    { return m_obj->getID(); }
		wsWindow *getObject() const		{ return m_obj; }

	private:
		friend class wsApplication;
		
		u32 m_type;
		u32 m_id;
		wsWindow *m_obj;
		
		wsEvent *m_pNext;
		wsEvent *m_pPrev;
		
};	// wsEvent




//------------------------------------
// the application object
//------------------------------------

class wsApplication : public wsWindow
{
	public:
		
		~wsApplication();
		wsApplication();

		// You must call Initialize and provide a Create() method.
		// The Create() method will be called to allow you to create
		// the initial set of windows and controls, thereafter you
		// need only call timeSlice() in a loop.
		
		void Initialize(
			CScreenDeviceBase *pScreen,
			CTouchScreenBase  *pTouch,
			CMouseDevice      *pMouse);
		
		void Create();
			// You must provide wsApplication::Create()
			
		void timeSlice();
			// it's called timeSlice() to prevent confusion
			// with the wsWindows::update() functions. 

		wsTopLevelWindow *getTopWindow() const { return m_pTopWindow; }
		
		// public to wsWindows
		// not intended for client use
		
		void setTouchFocus(wsWindow *win) {m_pTouchFocus = win;}
			// the wsWindow, if any, currently being pressed
		void addTopLevelWindow(wsTopLevelWindow *pWindow);
		void removeTopLevelWindow(wsTopLevelWindow *pWindow);
		
		void addEvent(wsEvent *event);
		
	private:
		
		wsEvent *m_pFirstEvent;
		wsEvent *m_pLastEvent;
		
		wsWindow *m_pTouchFocus;
		
		wsTopLevelWindow *m_pBottomWindow;
		wsTopLevelWindow *m_pTopWindow;
		
		CScreenDeviceBase *m_pScreen;
		CTouchScreenBase  *m_pTouch;
		CMouseDevice      *m_pMouse;
		
		void mouseEventHandler(
			TMouseEvent event,
			unsigned buttons,
			unsigned x,
			unsigned y);
		static void mouseEventStub(
			void *pThis,
			TMouseEvent event,
			unsigned buttons,
			unsigned x,
			unsigned y);
	
		void touchEventHandler(
			TTouchScreenEvent event,
			unsigned id,
			unsigned x,
			unsigned y);
		static void touchEventStub(
			void *pThis,
			TTouchScreenEvent event,
			unsigned id,
			unsigned x,
			unsigned y);
		
		void onTouchEvent(
			unsigned x,
			unsigned y,
			bool down);
		
		u32 m_lastTouchUpdate;
		
};


#endif  // !_wsWindow_h