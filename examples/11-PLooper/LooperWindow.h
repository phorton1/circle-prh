
#ifndef _LooperWindow_h_
#define _LooperWindow_h_

#include <ws/ws.h>


class LooperWindow : public wsTopLevelWindow
{
	public:
		
		LooperWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye);
			
	private:
		
		wsMidiButton *m_pRecordButton;
		wsMidiButton *m_pPlayButton;
		wsMidiButton *m_pNextButton;
		wsMidiButton *m_pStopButton;		
		
		virtual u32 handleEvent(wsEvent *event);
	
};

#endif // !_LooperWindow_h_
