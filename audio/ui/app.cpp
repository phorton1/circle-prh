
#include "app.h"
#include <circle/logger.h>

#define log_name "app"


CApplication::CApplication() :
    window_main(this),
    window_status(this),
    window_record(this)
{
    cur_window = 0;

    LOG("ctor",0);
    showWindow(WINDOW_MAIN);
    // LOG("ctor finished",0);
}


CApplication::~CApplication()
{
    
}


void CApplication::showWindow(u16 num)
{
    // LOG("showWindow(%d)",num);
    if (cur_window)
    {
        cur_window->Hide();
    }
    switch (num)
    {
        case WINDOW_RECORD :
            cur_window = &window_record;
            break;
        case WINDOW_STATUS :
            cur_window = &window_status;
            break;
        default:
        case WINDOW_MAIN :
            cur_window = &window_main;
            break;
    }
    cur_window->Show();
}


