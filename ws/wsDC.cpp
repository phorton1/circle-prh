//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"
#include <circle/logger.h>
#define log_name "wsdc"


typedef u8 (*drawLineDriver)(void *pThat, s16 x0, s16 y0, s16 x1, s16 y1, wsColor color);
typedef u8 (*fillFrameDriver)(void *pThat, s16 x0, s16 y0, s16 x1, s16 y1, wsColor color);
typedef void (*pushPixelFxn)(void *pThat, wsColor color);
typedef pushPixelFxn (*fillAreaDriver)(void *pThat, s16 x0, s16 y0, s16 x1, s16 y1);

		 

void wsDC::fillScreen( wsColor color )
{
	fillFrame(0,0,m_xdim-1,m_ydim-1,color);
}


#define swapU16(i,j)  { u16 t = i; i=j; j=t; }


void wsDC::fillFrame( u16 x0, u16 y0, u16 x1, u16 y1, wsColor color )
{
	if ( x1 < x0 ) swapU16(x0,x1);
	if ( y1 < y0 ) swapU16(y0,y1);

	wsRect rect(x0,y0,x1,y1);
	rect.intersect(m_clip);
	if (rect.isEmpty())
		return;
	
	if ( m_opt_driver[OPT_DRIVER_FILL_FRAME])
	{
		((fillFrameDriver)m_opt_driver[OPT_DRIVER_FILL_FRAME])
			(m_pScreen,rect.xs,rect.ys,rect.xe,rect.ye,color);
		return;
	}
	
	for (u16 y=rect.ys; y<=rect.ye; y++ )
	{
	   for (u16 x=rect.xs; x<=rect.xe; x++ )
	   {
		  setPixel(x,y,color);
	   }
	}
}



void wsDC::drawLine( u16 x0, u16 y0, u16 x1, u16 y1, wsColor color )
{
	wsRect rect(x0,y0,x1,y1);
	rect.intersect(m_clip);
	if (rect.isEmpty())
		return;

	if ( m_opt_driver[OPT_DRIVER_DRAW_LINE])
	{
		// clipping not applied to optimized draw line.
		// which needs to be passed the clipping rectangle ..
		
		((drawLineDriver)m_opt_driver[OPT_DRIVER_DRAW_LINE])
			(m_pScreen, x0,y0,x1,y1,color);
		return;
	}

	s16 dx = x1 - x0;
	s16 dy = y1 - y0;
	s16 dxabs = (dx>0) ? dx : -dx;
	s16 dyabs = (dy>0) ? dy : -dy;
	s16 sgndx = (dx>0) ? 1  : -1;
	s16 sgndy = (dy>0) ? 1  : -1;
	s16 x = dyabs >> 1;
	s16 y = dxabs >> 1;
	s16 drawx = x0;
	s16 drawy = y0;
	
	setPixel(drawx, drawy, color);

	if( dxabs >= dyabs )
	{
		for (s16 n=0; n<dxabs; n++)
		{
			y += dyabs;
			if (y >= dxabs)
			{
				y -= dxabs;
				drawy += sgndy;
			}
			drawx += sgndx;
			if (rect.intersects(drawx,drawy))
				setPixel(drawx, drawy, color);
		}
	}
	else
	{
		for (s16 n=0; n<dyabs; n++)
		{
			x += dxabs;
			if (x >= dyabs)
			{
				x -= dyabs;
				drawx += sgndx;
			}
			drawy += sgndy;
			if (rect.intersects(drawx,drawy))
				setPixel(drawx, drawy, color);
		}
	}
}



void wsDC::drawFrame( u16 x0, u16 y0, u16 x1, u16 y1, wsColor color )
{
   drawLine(x0,y0,x1,y0,color);
   drawLine(x0,y1,x1,y1,color);
   drawLine(x0,y0,x0,y1,color);
   drawLine(x1,y0,x1,y1,color);	
}




void wsDC::fillRoundFrame( u16 x0, u16 y0, u16 x1, u16 y1, u16 r, wsColor color )
{
	
}

void wsDC::wsDC::drawMesh( u16 x0, u16 y0, u16 x1, u16 y1, wsColor color )
{
	
}



void wsDC::drawRoundFrame( u16 x0, u16 y0, u16 x1, u16 y1, u16 r, wsColor color )
{
	
}

void wsDC::drawCircle( u16 x, u16 y, u16 r, wsColor color )
{
	
}

void wsDC::fillCircle( u16 x, u16 y, u16 r, wsColor color )
{
	
}

void wsDC::drawArc( u16 x, u16 y, u16 r, u8 s, wsColor color )
{
	
}


void wsDC::_putChar( char chr, u16 x, u16 y, wsColor fc, wsColor bc, const wsRect &clip )
{
	// set the background and foreground to distinctive colors
	// to prove it's only drawing the invalidated region
	//
	//	if (!m_invalid.isEmpty())
	//	{
	//		fc = wsMAGENTA;
	//		bc = wsBLACK;
	//	}

	u8 bt = (u8) chr;
	switch ( bt )
	{
		case 0xF6: bt = 0x94; break; // ö
		case 0xD6: bt = 0x99; break; // Ö
		case 0xFC: bt = 0x81; break; // ü
		case 0xDC: bt = 0x9A; break; // Ü
		case 0xE4: bt = 0x84; break; // ä
		case 0xC4: bt = 0x8E; break; // Ä
		case 0xB5: bt = 0xE6; break; // µ
		case 0xB0: bt = 0xF8; break; // °
	}
	
	assert(m_pFont);
	if (bt < m_pFont->start_char || bt > m_pFont->end_char)
		return;

	u16	char_width = m_pFont->char_width;
	if (!char_width)
		return;
	u16 char_height = m_pFont->char_height;

	wsRect rect(x,y, x+char_width-1, y+char_height-1);
	rect.intersect(clip);
	if (rect.isEmpty())
		return;

	u16 bn = char_width >> 3;
	if (char_width % 8)
		bn++;
	u32 index = (bt - m_pFont->start_char)* char_height * bn;
	
	u16 yo = y;
		
	if ( m_opt_driver[OPT_DRIVER_FILL_AREA])
	{
		pushPixelFxn pushPixel = ((fillAreaDriver)m_opt_driver[OPT_DRIVER_FILL_AREA])
			(m_pScreen, rect.xs, rect.ys, rect.xe, rect.ye);
		
		for (u16 j=0; j<char_height; j++)
		{
			u16 xo = x;
			u16 c = char_width;
			for (u16 i=0; i<bn; i++)
			{
				u8 b = m_pFont->p[index++];
				for(u16 k=0; (k<8) && c; k++)
				{
					if( b & 0x01 )
					{
						if (rect.intersects(xo,yo))
							pushPixel(m_pScreen,fc);
					}
					else
					{
						if (rect.intersects(xo,yo))
							pushPixel(m_pScreen,bc);
					}
					b >>= 1;
					xo++;
					c--;
				}
			}
			yo++;
		}
	}
	else	// not accelerated
	{
		for	(u16 j=0; j<char_height; j++)
		{
			u16 xo = x;
			u16 c = char_width;
			for (u16 i=0; i<bn; i++)
			{
				u8 b = m_pFont->p[index++];
				for (u16 k=0;(k<8) && c; k++)
				{
					if (b & 0x01)
					{
						if (rect.intersects(xo,yo))
							setPixel(xo,yo,fc);
					}
					else
					{
						if (rect.intersects(xo,yo))
							setPixel(xo,yo,bc);
					}
					b >>= 1;
					xo++;
					c--;
				}
			}
			yo++;
		}
	}
}


void wsDC::putString( u16 x, u16 y, const char* str )
{
	u16 xp = x;
	u16 yp = y;
	if (m_clip.isEmpty())
		return;
	
	assert(m_pFont);
	while ( *str != 0 )
	{
		char chr = *str++;
		if (chr < m_pFont->start_char || chr > m_pFont->end_char)
			continue;
		
		if ( chr == '\n' )
		{
			xp = m_xdim;
			continue;
		}
		u16 cw = m_pFont->char_width;
			// no proportional font support yt
			// gui->font.widths ? gui->font.widths[chr - gui->font.start_char] : gui->font.char_width;
		
		if ( xp + cw > m_xdim )
		{
			xp = x;
			yp += m_pFont->char_height + m_vspace;
		}
		
		_putChar(chr, xp, yp, m_fore_color, m_back_color, m_clip);
		
		xp += cw + m_hspace;
	}	
}


void wsDC::putText(
	wsColor bc,
	wsColor fc,
	const wsRect &area,
	wsAlignType align,
	s16     hspace,
	s16     vspace,
	const char *text)
{
	if (!m_pFont->p)
		return;
	if (!text || !*text)
		return;
	
	wsRect rect(area);
	rect.intersect(m_clip);
	if (rect.isEmpty())
		return;
		
	u16 xs=area.xs;
	u16 ys=area.ys;
	u16 xe=area.xe;
	u16 ye=area.ye;
	
	s16 char_width = m_pFont->char_width;
	s16 char_height = m_pFont->char_height;
	
	u16 rc = 1;
	const char *c = text;
	while (*c)
	{
		if (*c == '\n')
			rc++;
		c++;
	}
		
	s16 yp = 0;
	if (align & (ALIGN_V_CENTER | ALIGN_V_BOTTOM))
	{
		yp = ye - ys + 1;
		yp -= char_height * rc;
		yp -= vspace * (rc-1);
		// if (yp < 0)
		//	return;
	}
	if (align & ALIGN_V_CENTER)
		yp >>= 1;
	yp += ys;
	if (yp < 0)
		yp = 0;
		
	const char *str = text;
	while (1)
	{
		s16 sl = 0;
		s16 wl = 0;
		c = text;
		while (*c  && (*c != '\n'))
		{
			if (*c < m_pFont->start_char || *c > m_pFont->end_char)
			{
				c++;
				continue;
			}
			
			sl++;
			wl += char_width + hspace;
			c++;
		}
		wl -= hspace;
		
		s16 xp = xe - xs + 1;
		xp -= wl;
		// if (xp < 0)
			// return;
		
		if (align & ALIGN_H_LEFT)
			xp = 0;
		else if (align & ALIGN_H_CENTER)
			xp >>= 1;
		xp += xs;
		if (xp < 0)
			xp = 0;
			
		while((*str != '\n'))
		{
			char chr = *str++;
			if (chr == 0)
				return;
			
			wsRect chr_rect(xp,yp,xp+char_width-1,yp+char_height-1);
			if (chr_rect.intersects(rect))
				_putChar(chr,xp,yp,fc,bc,rect);
			xp += char_width + hspace;
		}
		str++;
		yp += char_height + vspace;
	}	
}




void wsDC::consolePutString( char* str )
{
	
}

void wsDC::consoleSetArea( u16 xs, u16 ys, u16 xe, u16 ye )
{
	
}

void wsDC::consoleSetForecolor( wsColor color )
{
	
}

void wsDC::consoleSetBackcolor( wsColor color )
{
	
}


void wsDC::draw3DFrame( u16 xs, u16 ys, u16 xe, u16 ye, const wsColor *pColor )
{
   drawLine(xs, ys  , xe-1, ys  , *pColor++);
   drawLine(xs, ys+1, xs  , ye-1, *pColor++);
   drawLine(xs, ye  , xe  , ye  , *pColor++);
   drawLine(xe, ys  , xe  , ye-1, *pColor++);

   drawLine(xs+1, ys+1, xe-2, ys+1, *pColor++);
   drawLine(xs+1, ys+2, xs+1, ye-2, *pColor++);
   drawLine(xs+1, ye-1, xe-1, ye-1, *pColor++);
   drawLine(xe-1, ys+1, xe-1, ye-2, *pColor++);

   drawLine(xs+2, ys+2, xe-3, ys+2, *pColor++);
   drawLine(xs+2, ys+3, xs+2, ye-3, *pColor++);
   drawLine(xs+2, ye-2, xe-2, ye-2, *pColor++);
   drawLine(xe-2, ys+2, xe-2, ye-3, *pColor);
}


void wsDC::driverRegister(void *pUnusedScreen, u8 type, void *driver)
{
	LOG("driverRegister(%d)",type);
	assert(type < NUM_OPT_DRIVERS);
	if (type < NUM_OPT_DRIVERS)
		m_opt_driver[type] = driver;
}


void wsDC::driverRegisterStub(void *pThis, void *pUnusedScreen, u8 type, void *driver)
{
	assert(pThis);
	((wsDC *)pThis)->driverRegister(pUnusedScreen,type,driver);
}
