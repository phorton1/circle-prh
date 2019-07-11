//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsCheckbox_h
#define _wsCheckbox_h

#include "wsControl.h"

//---------------------------
// wsCheckbox
//---------------------------
// checkboxes have a fixed size

#define CHECKBOX_SIZE   20

#define CHB_STATE_RELEASED                            0x0000
#define CHB_STATE_PRESSED                             0x0001
#define CHB_STATE_CHECKED                             0x0002

#define CHB_STYLE_2D                                  0x0001
#define CHB_STYLE_3D                                  0x0002
#define CHB_STYLE_TOGGLE_COLORS                       0x0004
#define CHB_STYLE_USE_ALTERNATE_COLORS                0x0008


class wsCheckbox : public wsControl
{
	public:
	
		~wsCheckbox() {}
		wsCheckbox(
				wsWindow *pParent,
				u16 id,
				u8 checked,
				s32 xs,
				s32 ys,
				u16 cstyle=0,
				u32 addl_wstyle=0) :
			wsControl(pParent,id,xs,ys,xs+CHECKBOX_SIZE-1,ys+CHECKBOX_SIZE-1,
				addl_wstyle |
  				WIN_STYLE_TOUCH |
				WIN_STYLE_CLICK | (
				(cstyle & CHB_STYLE_2D) ? WIN_STYLE_2D :
				(cstyle & CHB_STYLE_3D) ? WIN_STYLE_3D : 0)),
			m_checkbox_style(cstyle)
		{
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
			setBit(m_state,WIN_STATE_DRAW);
		}
		
		void setAltBackColor(wsColor color)  {m_alt_back_color = color;}
		void setAltForeColor(wsColor color)  {m_alt_fore_color = color;}

	protected:
	
		u8 m_checkbox_state;
		u16 m_checkbox_style;
		wsColor m_alt_back_color;
		wsColor m_alt_fore_color;
		
		virtual void onDraw();
		virtual void onUpdateClick();
		virtual void onUpdateTouch(bool touched);
		

};	// wsCheckbox


#endif  // !_wsCheckbox_h 
