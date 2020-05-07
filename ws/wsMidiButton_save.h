//
// a Button that can be tied to a Midi Event

#ifndef _wsMidiButton_h_
#define _wsMidiButton_h_

#include "wsControl.h"
#include <system/std_kernel.h>
#include <system/midiEvent.h>


class wsMidiButton : public wsControl
	// these buttons differ from regular buttons in
	// that they generate events upon being pressed,
	// not upon being released.
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
		
	private:
		
		u8 m_button_state;
		u8 m_pressed;
		wsColor m_pressed_back_color;
		wsColor m_pressed_fore_color;
		
		virtual void onDraw();
		// virtual void onUpdateClick();
		virtual void onUpdateTouch(bool touched);
		
		static void staticHandleMidiEvent(void *pObj, midiEvent *event);
		void handleMidiEvent(midiEvent *event);
		
};	// midiButton


#endif  // !_awsLoopTrackButton_h
