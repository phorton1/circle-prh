
#ifndef _uiTrack_h
#define _uiTrack_h

#include <ws/wsControl.h>
#include <system/midiEvent.h>
#include "looper.h"


#define NUM_LTB_ROWS   3
#define NUM_LTB_COLS   4


class uiTrack : public wsWindow
{
	public:
	
		~uiTrack() {}
		
		uiTrack(
				u8 track_num,
				wsWindow *pParent,
				u16 id,
				s32 xs,
				s32 ys,
				s32 xe,
				s32 ye);
 
	protected:
		
		u8 m_track_num;
		loopTrack *m_pLoopTrack;

		bool m_selected;
		
		virtual void onDraw();
		virtual void updateFrame();
		
};	// _uiTrack_h


#endif  // !_uiClip_h
