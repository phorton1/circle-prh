
#ifndef _status_bar_h_
#define _status_bar_h_

#include <ugui/uguicpp.h>
#include "app.h"

class CStatusBar 
{
public:
    
	CStatusBar(CWindow *win, appWindowNum num);   

    void updateStatusText(const char *text);
    
private:
    
    CWindow *m_win;
    appWindowNum m_window_num;
    
    CTextbox  *m_statusText;
    CMenu     *m_main_menu;
    
};

#endif  // !_status_bar_h_
