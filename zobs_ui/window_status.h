
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
    bool         m_bStarted;
    
    void init();
    void draw();
    void show(s16 x, s16 y, const char *pMessage, ...);
    
    
};


#endif  // !_window_status_h_
