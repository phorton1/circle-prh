
#ifndef _uiStatusBar_h_
#define _uiStatusBar_h_

#include <ws/ws.h>


class uiStatusBar : public wsWindow
{
	public:	

		uiStatusBar(wsWindow *pParent, u16 id, s32 xs, s32 ys, s32 xe, s32 ye);
		virtual void updateFrame();
		virtual void onDraw();

	private:
		
		u32 m_blocks_used;
		u32 m_blocks_free;
		u16 m_num_tracks;
		u16 m_num_clips;
	
};


#endif  // !_uiStatusBar_h_