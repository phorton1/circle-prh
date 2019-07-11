//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsRect_h
#define _wsRect_h

#include <circle/types.h>
#include "wsDefines.h"

//------------------------------------
// wsRect
//------------------------------------

extern void print_rect(const char *name, const wsRect *rect);

class wsRect
{
public:

	// expects normalized rectangles
	// empty is indicated by end < start
	// cannonical(1,1,0,0)
	
	wsRect();
	wsRect(const wsRect &rect);
	wsRect(u16 x0, u16 y0, u16 x1, u16 y1);
	wsRect(s32 x0, s32 y0, s32 x1, s32 y1);
	
	wsRect &assign(const wsRect &rect);
	wsRect &assign(u16 x0, u16 y0, u16 x1, u16 y1);
	wsRect &assign(s32 x0, s32 y0, s32 x1, s32 y1);

	bool isEmpty() const;
	s32 getWidth() const;
	s32 getHeight() const;
	
	wsRect &empty();	
	wsRect &expand(const wsRect &rect);
	wsRect &intersect(const wsRect &rect);
	wsRect &makeRelative(const wsRect &rect);
	
	bool intersects(u16 x, u16 y) const;
	bool intersects(s32 x, s32 y) const;
	bool intersects(const wsRect &rect) const;

	s32 xs;
	s32 ys;
	s32 xe;
	s32 ye;
};


#endif  // !_wsRect_h
