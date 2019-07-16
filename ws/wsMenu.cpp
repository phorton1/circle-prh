#include "wsMenu.h"
#include "wsEvent.h"
#include "wsTopWindow.h"
#include "wsApp.h"


#define MENU_MARGIN     4
#define SINGLE_POP_WINDOW_ID    32000


class wsMenuWindow : public wsTopLevelWindow
{
public:

    wsMenuWindow(wsApplication *pApp, s32 xs, s32 ys, s32 xe, s32 ye) :
        wsTopLevelWindow(
            pApp,
            SINGLE_POP_WINDOW_ID,
            xs,ys,xe,ye,
            WIN_STYLE_POPUP |
            WIN_STYLE_2D)
    {
        // uhm, have to override the default ctor behavior
        // so the window starts as invisible and not the top window,
        // or else (a) calling hide() would invalidate the region, and
        // (b) it gets events when it's not the top (visible) window.
        
        m_state &= ~WIN_STATE_VISIBLE;
		pApp->removeTopLevelWindow(this);
        
    }
        
    virtual u32 handleEvent(wsEvent *event)
    {
        printf("menu handle_event()\n");
        if (event->getEventType() == EVT_TYPE_BUTTON)
        {
            hide();
            
            
            // you cannot call addEvent() from handleEvent() !!!
            //            
            // it cannot just get added to the list of events
            // being generated as they are also simultaneously
            // being dispatched!
            //
            // addPendingEvent() is not implemented yet!
            // so we send it directly to the next up top window ..

            return m_pParent->getTopWindow()->handleEvent(event);
        }
        return wsTopLevelWindow::handleEvent(event);
    }
    
    void incHeight(s32 inc)
    {
        m_rect.ye += inc;
    }
   
};




//-----------------------------------
// wsMenu
//-----------------------------------

wsMenu::wsMenu(
        wsWindow *pParent,
        u16 id,
        const char *text,
        s32 xs,
        s32 ys,
        s32 xe,
        s32 ye,
        s32 dw,
        u16 bstyle,
        u32 addl_wstyle) :
    wsButton(pParent,
        id,
        text,
        xs + MENU_MARGIN,
        ys + MENU_MARGIN,
        xe - MENU_MARGIN,
        ye - MENU_MARGIN,
        bstyle,
        addl_wstyle),
    m_item_height(ye-ys+1),
    m_dropdown_width(dw),
    m_num_choices(0)
{
    m_pPopupWin = new wsMenuWindow(pParent->getApplication(),
        xs,ye,xs + dw-1, ye+5);
    m_pPopupWin->setBackColor(wsDODGER_BLUE);
    setBackColor(wsLIGHT_BLUE);
}


void wsMenu::addChoice(u16 id, const char *text)
{
    // printf("addchoice(%d,%s)\n",id,text);
    s16 ypos = m_num_choices * m_item_height;
    
    // create the button
    
    wsButton *b = new wsButton(
        m_pPopupWin,
        id,
        text,
        MENU_MARGIN,
        ypos + MENU_MARGIN,
        m_dropdown_width - MENU_MARGIN - 1,
        ypos + m_item_height - MENU_MARGIN - 1);
    
    b->setBackColor(wsLIGHT_BLUE);
    m_num_choices++;

    // expand the popup window
    
    m_pPopupWin->incHeight(m_item_height);
}



void wsMenu::popup()
{
    m_pPopupWin->show();
}


