
#ifndef _window_status_h_
#define _window_status_h_

#include <audio/Audio.h>
#include <ugui/uguicpp.h>
#include "menu.h"


class CStatusWindow : public CWindow
{
public:
    
	CStatusWindow(CApplication *app);
	~CStatusWindow(void);
    
private:
    
	void Callback(UG_MESSAGE *pMsg);
    
    CApplication *m_pApp;
    CTitlebar    *m_pTitlebar;
    
};


#endif  // !_window_status_h_
