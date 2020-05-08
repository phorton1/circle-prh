
#ifndef _uiWindow_h_
#define _uiWindow_h_

#include <ws/ws.h>

#define ID_WIN_LOOPER    	100

class uiWindow : public wsTopLevelWindow
{
	public:
		
		uiWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye);
			
	private:
		
		virtual u32 handleEvent(wsEvent *event);
	
};

#endif // !_uiWindow_h_
