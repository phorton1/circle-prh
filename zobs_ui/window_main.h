
#ifndef _window_main_h_
#define _window_main_h_

#include <audio/Audio.h>
#include <ugui/uguicpp.h>
#include "menu.h"


class CMainWindow : public CWindow
{
public:
    
	CMainWindow(CApplication *app);
	~CMainWindow(void);
    
private:
    
	void Callback(UG_MESSAGE *pMsg);
    
    CApplication *m_pApp;
    CTitlebar    *m_pTitlebar;
    
};


#endif  // !_window_main_h_
