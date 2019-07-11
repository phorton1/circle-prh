//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsControl_h
#define _wsControl_h

#include "wsWindow.h"

//-------------------------------------------
// wsControl
//-------------------------------------------

class wsControl : public wsWindow
{
	public:
	
		~wsControl() {}
		
		wsControl(wsWindow *pParent, u16 id, s32 xs, s32 ys, s32 xe, s32 ye, u32 style=0) :
			wsWindow(pParent,id,xs,ys,xe,ye,style) {}
};



//---------------------------
// wsStaticText
//---------------------------

class wsStaticText : public wsControl
{
	public:
	
		~wsStaticText() {}
		
		wsStaticText(wsWindow *pParent, u16 id, const char *text, s32 xs, s32 ys, s32 xe, s32 ye) :
			wsControl(pParent,id,xs,ys,xe,ye)
		{
			if (text)
				m_text = text;
		}
		
	protected:
	
		virtual void onDraw();
	
};



//---------------------------
// wsImage
//---------------------------

class wsImage : public wsControl
{
	public:
	
		wsImage(wsWindow *pParent, u16 id, u8 value, s32 x, s32 y, s32 xe, s32 ye, u32 style=0);
		~wsImage() {}
};



#endif  // !_wsControl_h
