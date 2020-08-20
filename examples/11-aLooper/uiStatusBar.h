
#ifndef _uiStatusBar_h_
#define _uiStatusBar_h_

#include <ws/ws.h>


class uiStatusBar : public wsWindow
{
	public:

		uiStatusBar(wsWindow *pParent, u16 id, s32 xs, s32 ys, s32 xe, s32 ye);
		virtual void updateFrame();
		virtual void onDraw();

};


#endif  // !_uiStatusBar_h_