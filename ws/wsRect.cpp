//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsWindow.h"

void print_rect(const char *name, const wsRect *rect)
{
	printf("%s(%d,%d,%d,%d)\n",name,rect->xs,rect->ys,rect->xe,rect->ye);
}


wsRect::wsRect()
{
	xs = 0;
	ys = 0;
	xe = 0;
	ye = 0;
}

wsRect::wsRect(const wsRect &pRect)
{
	xs = pRect.xs;
	ys = pRect.ys;
	xe = pRect.xe;
	ye = pRect.ye;
}

wsRect::wsRect(u16 x0, u16 y0, u16 x1, u16 y1)
{
	xs = x0;
	ys = y0;
	xe = x1;
	ye = y1;
}

wsRect::wsRect(s32 x0, s32 y0, s32 x1, s32 y1)
{
	xs = x0;
	ys = y0;
	xe = x1;
	ye = y1;
}

wsRect &wsRect::empty() 
{
	xs = 1;
	ys = 1;
	xe = 0;
	ye = 0;
 	return *this;
}

wsRect &wsRect::assign(u16 x0, u16 y0, u16 x1, u16 y1)
{
	xs = x0;
	ys = y0;
	xe = x1;
	ye = y1;
	return *this;
}

wsRect &wsRect::assign(s32 x0, s32 y0, s32 x1, s32 y1)
{
	xs = x0;
	ys = y0;
	xe = x1;
	ye = y1;
	return *this;
}

wsRect &wsRect::assign(const wsRect &rect)
{
	xs = rect.xs;
	ys = rect.ys;
	xe = rect.xe;
	ye = rect.ye;
	return *this;
}


bool wsRect::isEmpty() const
{
	return (xs > xe) ||  (ys > ye);
}


s32 wsRect::getWidth() const
{
	if (xe < xs) return 0;
	return xe-xs+1;
}


s32 wsRect::getHeight() const
{
	if (ye < ys) return 0;
	return ye-ys+1;
}


wsRect &wsRect::makeRelative(const wsRect &rect)
{
	xs += rect.xs;
	ys += rect.ys;
	xe += rect.xs;
	ye += rect.ys;
	return *this;
}

bool wsRect::intersects(u16 x, u16 y) const
{
	return (x >= xs) && (y >= ys) && (x <= xe) && (y <= ye);
}

bool wsRect::intersects(s32 x, s32 y) const
{
	return (x >= xs) && (y >= ys) && (x <= xe) && (y <= ye);
}


bool wsRect::intersects(const wsRect &rect) const
{
	wsRect arect(rect);
	arect.intersect(*this);
	return !arect.isEmpty();
}


wsRect &wsRect::intersect(const wsRect &rect)
{
	if (rect.isEmpty())
	{
		empty();
	}
	else if (!isEmpty())
	{
		if (xs > rect.xe)
			empty();
		else if (ys > rect.ye)
			empty();
		else if (xe < rect.xs)
			empty();
		else if (ye < rect.ys)
			empty();
		else
		{
			if (rect.xs > xs)
				xs = rect.xs;
			if (rect.ys > ys)
				ys = rect.ys;
			if (rect.xe < xe)
				xe = rect.xe;
			if (rect.ye < ye)
				ye = rect.ye;
		}
	}
	
	return *this;
}


wsRect &wsRect::expand(const wsRect &rect)
{
	if (isEmpty())
	{
		assign(rect);
	}
	else
	{
		if (rect.xs < xs)
			xs = rect.xs;
		if (rect.ys < ys)
			ys = rect.ys;
		if (rect.xe > xe)
			xe = rect.xe;
		if (rect.ye > ye)
			ye = rect.ye;
	}
	return *this;
}



