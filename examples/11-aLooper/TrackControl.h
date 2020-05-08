
#ifndef _TrackControl_h
#define _TrackControl_h

#include <ws/wsControl.h>
#include <system/midiEvent.h>
#include "Looper.h"


#define NUM_LTB_ROWS   3
#define NUM_LTB_COLS   4


class TrackControl : public wsWindow
{
	public:
	
		~TrackControl() {}
		
		TrackControl(
				u8 track_num,
				wsWindow *pParent,
				u16 id,
				s32 xs,
				s32 ys,
				s32 xe,
				s32 ye);
 
	protected:
		
		u8 m_track_num;
		LoopTrack *m_pLoopTrack;

		bool m_selected;
		
		virtual void onDraw();
		virtual void updateFrame();
		
};	// _TrackControl_h


#endif  // !_ClipButton_h
