
#ifndef _uiClip_h
#define _uiClip_h

#include <ws/ws.h>
#include <system/midiEvent.h>
#include "looper.h"


#define NUM_LTB_ROWS   3
#define NUM_LTB_COLS   4


class uiClip : public wsWindow
{
	public:
	
		~uiClip() {}
		
		uiClip(
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
		loopClip *m_pLoopClip;
		
		u8 m_pressed;

		u8  m_clip_state;
		u32 m_cur_block;
		u32 m_num_blocks;
		
		virtual void onDraw();
		virtual void updateFrame();
		virtual void onUpdateTouch(bool touched);
		
		static void staticHandleMidiEvent(void *pObj, midiEvent *event);
		void handleMidiEvent(midiEvent *event);
		
		
};	// uiClip


#endif  // !_uiClip_h
