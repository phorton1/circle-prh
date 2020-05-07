
#ifndef _LoopWindow_h_
#define _LoopWindow_h_

#include <ws/ws.h>


class LoopWindow : public wsTopLevelWindow
{
	public:
		
		LoopWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye);
			
	private:
		
		virtual u32 handleEvent(wsEvent *event);
	
};

#endif // !_LoopWindow_h_
