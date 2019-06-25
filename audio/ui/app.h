
#ifndef _ui_app_h_
#define _ui_app_h_

// The main application maintains state about which window is open
// and facilitates window context changes.  It is not itself a
// window or control object, but knows about, constructs, and
// shows and hides  all the main windows.

#include <audio/Audio.h>
#include <ugui/uguicpp.h>
#include "window_main.h"
#include "window_status.h"
#include "window_record.h"

#define WINDOW_MAIN   0
#define WINDOW_STATUS 1
#define WINDOW_RECORD 2
#define NUM_WINDOWS   3


class CApplication 
{
public:
    
	CApplication();
    ~CApplication();
    
    void showWindow(u16 num);
    u16 getNumWindows()   { return NUM_WINDOWS; }
    const char *getWindowName(u16 num)
    {
        if (num == WINDOW_RECORD)  return "RECORD";
        if (num == WINDOW_STATUS)  return "STATUS";
        return "MAIN";
   }
    
private:
    
    CMainWindow   window_main;         
    CStatusWindow window_status;       
    CRecordWindow window_record;       
    
    CWindow       *cur_window;
    
};

#endif  // !_ui_app_h_
