//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsButton_h
#define _wsButton_h

#include "wsControl.h"

//---------------------------
// wsButton
//---------------------------

#define BTN_STATE_RELEASED                            0x0000
#define BTN_STATE_PRESSED                             0x0001

#define BTN_STYLE_2D                                  0x0001
#define BTN_STYLE_3D                                  0x0002
#define BTN_STYLE_TOGGLE_COLORS                       0x0004
#define BTN_STYLE_USE_ALTERNATE_COLORS                0x0008
#define BTN_STYLE_TOGGLE_VALUE                        0x0010


class wsButton : public wsControl
{
	public:
	
		~wsButton() {}
		
		wsButton(
				wsWindow *pParent,
				u16 id,
				const char *text,
				s32 xs,
				s32 ys,
				s32 xe,
				s32 ye,
				u16 bstyle=0,
				u32 addl_wstyle=0);

		bool isPressed() const { return m_button_state & BTN_STATE_PRESSED; }
		void setAltBackColor(wsColor color)  {m_alt_back_color = color;}
		void setAltForeColor(wsColor color)  {m_alt_fore_color = color;}

	protected:
	
		u8 m_button_state;
		u16 m_button_style;
		wsColor m_alt_back_color;
		wsColor m_alt_fore_color;
		
		virtual void onDraw();
		virtual void onUpdateClick();
		virtual void onUpdateTouch(bool touched);
		
};	// wsButton


#endif  // !_wsButton_h
