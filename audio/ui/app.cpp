
#include "app.h"
#include <circle/logger.h>

#define log_name "app"


CApplication::CApplication() :
    window_main(this),
    window_status(this),
    window_record(this)
{
    showWindow(WINDOW_STATUS);
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


