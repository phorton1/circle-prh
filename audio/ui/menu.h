#ifndef _main_menu_h_
#define _main_menu_h_

#include "vu_meter.h"
#include <ugui/uguicpp.h>

#define APP_TOP_MARGIN      40

// these textbox ids are added to the main windows

#define ID_APP_FILLER       255
#define ID_APP_TITLE        254
#define ID_APP_STATUS       253
#define ID_MENU_MAIN        252
#define ID_STATUS_VU1       251
#define ID_STATUS_VU2       250


class CApplication;

typedef void (*menuCallback)(void *, u8 menu_id);


class CMenu : public CButton
{
public:
    
	CMenu(
        void *param,        // your "this"
        menuCallback cb,    // a call back method to receive an id
        CWindow *win,       // the parent window for the main button
        u8 id,              // id of the main button
        s16 x,              // dimensions of the main button
        s16 y,              // relative to the parent window
        s16 w,
        s16 h,
        const char *text,    // text of the main button
        s16 dw);             // width of drop down box

    void addChoice(u8 id, const char *text);
    void popup();
    void popdown();
    bool isShowing();

    bool Callback(UG_MESSAGE *pMsg);
        // called UP from owning window
        // returns true if it's a menu button press
    
protected:
    
    CWindow    *m_pWin;
    CWindow    *m_pPopupWin;
    s16         m_item_height;
    s16         m_dropdown_width;
    u8          m_num_choices;
    
};




class CTitlebar 
{
public:
    
	CTitlebar(CWindow *win, CApplication *app, u8 window_num);
    
    bool Callback(UG_MESSAGE *pMsg);
        // called up from owning window
        // we call menu, which returns true if it's a menu button press
        // otherwise, we check for activate and de-activate (status, vumeter, etc)

    void setTitleText(const char *text);
    void setStatusText(const char *text);
    
private:

    CWindow      *m_pWin;
    CApplication *m_pApp;
    u8            m_window_num;
    
    CMenu        *m_pMenu;
    CTextbox     *m_pTitle;
    CTextbox     *m_pStatus;
    CVuMeter     *m_vu1;
    CVuMeter     *m_vu2;
    
    void menuCallback(u8 menu_id);
    static void menuCallbackStub(void *pThis, u8 menu_id);

};





#endif  // !_main_menu_h_
