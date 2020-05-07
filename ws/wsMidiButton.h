//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsMidiButton_h
#define _wsMidiButton_h

#include "wsControl.h"


#define BTN_STATE_RELEASED                            0x0000
#define BTN_STATE_PRESSED                             0x0001


class wsMidiButton : public wsControl
{
	public:
	
		~wsMidiButton() {}
		
		wsMidiButton(
				wsWindow *pParent,
				u16 id,
				const char *text,
				s32 xs,
				s32 ys,
				s32 xe,
				s32 ye);
 
		// void setPressedBackColor(wsColor color)  {m_alt_back_color = color;}
		// void setPressedForeColor(wsColor color)  {m_alt_fore_color = color;}

	protected:
	
		u8 m_pressed;
		u16 m_button_state;
		wsColor m_pressed_back_color;
		wsColor m_pressed_fore_color;
		
		virtual void onDraw();
		// virtual void onUpdateClick();
		virtual void onUpdateTouch(bool touched);
		
};	// wsMidiButton



#endif  // !_wsMidiButton_h
