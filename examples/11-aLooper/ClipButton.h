
#ifndef _ClipButton_h
#define _ClipButton_h

#include <ws/ws.h>
#include <system/midiEvent.h>
#include "Looper.h"


#define NUM_LTB_ROWS   3
#define NUM_LTB_COLS   4


class ClipButton : public wsWindow
{
	public:
	
		~ClipButton() {}
		
		ClipButton(
				u8 track_num,
				u8 clip_num,
				wsWindow *pParent,
				u16 id,
				s32 xs,
				s32 ys,
				s32 xe,
				s32 ye);
 
	protected:
		
		u8 m_track_num;
		u8 m_clip_num;
		LoopClip *m_pLoopClip;
		
		u8 m_pressed;

		u8  m_clip_state;
		u32 m_cur_block;
		u32 m_num_blocks;
		
		virtual void onDraw();
		virtual void updateFrame();
		virtual void onUpdateTouch(bool touched);
		
		static void staticHandleMidiEvent(void *pObj, midiEvent *event);
		void handleMidiEvent(midiEvent *event);
		
		
};	// ClipButton


#endif  // !_ClipButton_h
