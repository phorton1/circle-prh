//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.
// (c) Copyright 2019 Patrick Horton- no rights reserved, 

#ifndef _wsMenu_h_
#define _wsMenu_h_

#include "wsWindow.h"
#include "wsButton.h"

class wsMenuWindow;


class wsMenu : public wsButton
    // a wsMenu is just a button that generates an event within
    // the parent top level window.  That window must respond
    // to the event and call popup() to display the menu.
    // Subsequently, if an item is selected, the menu will
    // hide() itself, dispatching the menu button event to the
    // parent.   If the user clicks outside of the menu, it
    // will just hide itself, returning control to the parent
    // window.
{
public:
    

    wsMenu( wsWindow *pParent,
            u16 id,
            const char *text,
            s32 xs,
            s32 ys,
            s32 xe,
            s32 ye,
            s32 dw,
            u16 bstyle=0,
            u32 addl_wstyle=0);

    void addChoice(u16 id, const char *text);
    void popup();
    
protected:

    wsMenuWindow   *m_pPopupWin;
    s32             m_item_height;
    s32             m_dropdown_width;
    u8              m_num_choices;
    
};



#endif  // !_wsMenu_h_
