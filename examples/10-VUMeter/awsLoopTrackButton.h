//
// An object that combines the audio system with the ui system.
// A button representing one 'clip' in a looper


#ifndef _awsLoopTrackButton_h
#define _awsLoopTrackButton_h

#include <ws/wsControl.h>
#include <system/midiEvent.h>

#define NUM_LTB_ROWS   3
#define NUM_LTB_COLS   4


class awsLoopTrackButton : public wsControl
{
	public:
	
		~awsLoopTrackButton() {}
		
		awsLoopTrackButton(
				u8 row,
				u8 col,
				wsWindow *pParent,
				u16 id,
				s32 xs,
				s32 ys,
				s32 xe,
				s32 ye);
 
	protected:
		
		u8 m_row;
		u8 m_col;
		u8 m_ltb_state;
		u8 m_pressed;
		
		virtual void onDraw();
		virtual void onUpdateClick();
		virtual void onUpdateTouch(bool touched);
		
		static void staticHandleMidiEvent(void *pObj, midiEvent *event);
		void handleMidiEvent(midiEvent *event);
		
		
};	// awsLoopTrackButton


#endif  // !_awsLoopTrackButton_h
