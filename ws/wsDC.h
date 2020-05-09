//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsDC_h
#define _wsDC_h

#include "wsColor.h"
#include "wsFont.h"
#include "wsRect.h"
#include <circle/screen.h>


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
			m_ydim(m_pScreen->GetHeight()),
			m_clip(0,0,m_xdim-1,m_ydim-1)
			{
				m_pFont = 0;
				m_fore_color = 0;
				m_back_color = 0;
				m_align = 0;
				m_hspace = 0;
				m_vspace = 0;
				for (int i=0; i<NUM_OPT_DRIVERS; i++)
					m_opt_driver[i] = 0;
				m_invalid.empty();
			}

		CScreenDeviceBase *getScreen() 	{ return m_pScreen; }
		s32 getXDim( void ) const { return m_xdim; }
		s32 getYDim( void ) const { return m_ydim; }
		void setPixel(s32 x, s32 y, wsColor color)
			{ m_pScreen->SetPixel(x,y,color); }

		void fillScreen( wsColor color );
		void fillFrame( s32 x0, s32 y0, s32 x1, s32 y1, wsColor color );
		void drawLine( s32 x0, s32 y0, s32 x1, s32 y1, wsColor color );
		void drawFrame( s32 x0, s32 y0, s32 x1, s32 y1, wsColor color, u16 frame_width );
		void draw3DFrame( s32 xs, s32 ys, s32 xe, s32 ye, const wsColor *pColor );

		void fillRoundFrame( s32 x0, s32 y0, s32 x1, s32 y1, u16 r, wsColor color );
		void drawMesh( s32 x0, s32 y0, s32 x1, s32 y1, wsColor color );
		void drawRoundFrame( s32 x0, s32 y0, s32 x1, s32 y1, u16 r, wsColor color );
		void drawCircle( s32 x, s32 y, u16 r, wsColor color );
		void fillCircle( s32 x, s32 y, u16 r, wsColor color );
		void drawArc( s32 x, s32 y, u16 r, u8 s, wsColor color );

		void setFont( const wsFont *pFont )  { m_pFont = pFont; }
		void putString( s32 x, s32 y, const char* str );

		const wsRect &getClip()  { return m_clip; }
		void setClip(const wsRect &rect, bool invalid)
			// temporarily set to the window clipping region
			// for the next call(s)
		{
			m_clip.assign(rect);
			if (invalid && !m_invalid.isEmpty())
				m_clip.intersect(m_invalid);
		}
		const wsRect &getInvalid()			{ return m_invalid; }			
		void validate()						{ m_invalid.empty(); }
		void invalidate(const wsRect &rect)	{ m_invalid.expand(rect); }
			// sets an area of the screen as invalid (needs repainting)
			// so drawing methods only draw to the intersection of the
			// clipping region and the invalid region.  The invalid region
			// is cleared at the end of timeSlice() before sending events,
			// and then windows that intersect it are set to REDRAW at
			// the start of the loop.
		
		void putText(
			wsColor bc,
			wsColor fc,
			const wsRect &area,
			wsAlignType align,
			s16     hspace,
			s16     vspace,
			const char *text);

		void setForeColor( wsColor color )		{ m_fore_color = color; }
		void setBackColor( wsColor color )		{ m_back_color = color; }
		void setFontHSpace( s16 space )			{ m_hspace = space; }
		void setFontVSpace( s16 space ) 		{ m_vspace = space; }
		
		void consolePutString( char* str );
		void consoleSetArea( s32 xs, s32 ys, s32 xe, s32 ye );
		void consoleSetForecolor( wsColor color );
		void consoleSetBackcolor( wsColor color );
		
		void driverRegister(void *pUnusedScreen, u8 type, void *driver);
		static void driverRegisterStub(void *pThis, void *pUnusedScreen, u8 type, void *driver);
			// we don't need the pointer to the screen device inasmuch
			// as we already have it.  UGUI requires it because it is "C" code
			// calling static methods, divorced from the C++ object rst wrote.

	private:
		
		wsDC() {}

		void _putChar( char chr, s32 x, s32 y, wsColor fc, wsColor bc, const wsRect &clip);
		
		CScreenDeviceBase *m_pScreen;
		
		s32 m_xdim;
		s32 m_ydim;
		
		wsRect m_clip;
		wsRect m_invalid;

		const wsFont *m_pFont;
		wsColor m_fore_color;
		wsColor m_back_color;
		wsAlignType m_align;
		s16 m_hspace;
		s16 m_vspace;
		
		void *m_opt_driver[NUM_OPT_DRIVERS];
		
};	// wsDC



#endif  // !_wsDC_h
