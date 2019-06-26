#include "menu.h"
#include "app.h"
#include <audio/bcm_pcm.h>

#define MAIN_MENU_W     120
#define MAIN_MENU_H     40

#define STATUS_WIDTH    240
#define MENU_MARGIN     4



class CMenuWindow : public CWindow
{
public:

    CMenuWindow(
            void *param,
            menuCallback cb,
            UG_S16 xs,
            UG_S16 ys,
            UG_S16 xe,
            UG_S16 ye,
            UG_U8 style) :
        CWindow(xs,ys,xe,ye,style | WND_STYLE_POPUP),
        m_cb(cb),
        m_cb_param(param)
    {}

protected:
    
    menuCallback m_cb;
    void *m_cb_param;
    
    virtual void Callback(UG_MESSAGE *pMsg)
        // called directly on this real window only
    {
        if (pMsg->type  == MSG_TYPE_OBJECT &&
            pMsg->id    == OBJ_TYPE_BUTTON &&
            pMsg->event == OBJ_EVENT_PRESSED)
        {
            m_cb(m_cb_param,pMsg->sub_id);
        }
    }
};




//-----------------------------------
// CMenu
//-----------------------------------

CMenu::CMenu(
        void *param,        
        menuCallback cb,    
        CWindow *win,
        u8  id,
        s16 x,
        s16 y,
        s16 w,
        s16 h,
        const char *text,
        s16 dw) :
    CButton(
        win,
        id,
        x+MENU_MARGIN,
        y+MENU_MARGIN,
        x + w - 1 - MENU_MARGIN,
        y + h - 1 - MENU_MARGIN,
        text,
        BTN_STYLE_NO_BORDERS),
    m_pWin(win),
    m_item_height(h),
    m_dropdown_width(w)
{
    m_num_choices = 0;
    // printf("CMenu(%d, %d,%d,%d,%d, %s, %d)\n",id,x,y,w,h,text,dw);

    m_pPopupWin = new CMenuWindow(param, cb, x, h, x+dw-1, h+5, 0);   // WND_STYLE_3D);
    m_pPopupWin->SetBackColor(C_DODGER_BLUE);
    
    SetBackColor(C_LIGHT_BLUE);
}


void CMenu::addChoice(u8 id, const char *text)
{
    // printf("addchoice(%d,%s)\n",id,text);
    s16 ypos = m_num_choices * m_item_height;
    
    // create the button
    
    CButton *b = new CButton(
        m_pPopupWin,
        id,
        MENU_MARGIN,
        ypos + MENU_MARGIN,
        m_dropdown_width - MENU_MARGIN - 1,
        ypos + m_item_height - MENU_MARGIN - 1,
        text,
        BTN_STYLE_NO_BORDERS);
    
    b->Show();
    b->SetBackColor(C_LIGHT_BLUE);
    m_num_choices++;

    // expand the popup window
    
    UG_AREA area;
    m_pPopupWin->GetArea(&area);
    area.ye += m_item_height;
    m_pPopupWin->Resize(area.xs,area.ys,area.xe,area.ye);
}

bool CMenu::isShowing()
{
    bool rslt = m_pPopupWin->getUGWindow()->state & OBJ_STATE_VISIBLE ? 1 : 0;
    // printf("isShowing()=%d\n",rslt);
    return rslt;
}


void CMenu::popup()
{
    m_pPopupWin->Show();
}


void CMenu::popdown()
{
    m_pPopupWin->Hide();
    m_pPopupWin->getUGWindow()->state &= ~OBJ_STATE_VISIBLE;
}


bool CMenu::Callback(UG_MESSAGE *pMsg)
{
    if (pMsg->type  == MSG_TYPE_OBJECT &&
        pMsg->id    == OBJ_TYPE_BUTTON &&
        pMsg->event == OBJ_EVENT_PRESSED &&
        pMsg->sub_id == m_id)
    {
        if (isShowing())
            popdown();
        else
            popup();
        return true;
    }
   return false;
}



//-----------------------------------
// CTitlebar
//-----------------------------------

CTitlebar::CTitlebar(CWindow *win, CApplication *app, u8 window_num) :
    m_pWin(win),
    m_pApp(app),
    m_window_num(window_num)
{
    // printf("CTitlebar(%d) parent inner_width=%d\n",m_window_num,m_pWin->GetInnerWidth());

    // menu background
    
    CTextbox *filler = new CTextbox(
        m_pWin,
        ID_APP_FILLER,
        0,
        0,
        MAIN_MENU_W-1,
        MAIN_MENU_H-1,
        "title");
    filler->SetBackColor(C_DODGER_BLUE);
    
    // menu
    
    m_pMenu = new CMenu(
        this,
        menuCallbackStub,
        m_pWin,
        ID_MENU_MAIN,
        0,
        0,
        MAIN_MENU_W,
        MAIN_MENU_H,
        m_pApp->getWindowName(m_window_num),
        MAIN_MENU_W);
    for (u16 num=0; num<m_pApp->getNumWindows(); num++)
    {
        if (num != window_num)
            m_pMenu->addChoice(num, m_pApp->getWindowName(num));
    }
    
    // title
    
    s16 width = UG_GetXDim();
    s16 title_width = width - MAIN_MENU_W - STATUS_WIDTH;
    m_pTitle = new CTextbox(
        m_pWin,
        ID_APP_TITLE,
        MAIN_MENU_W,
        0,
        MAIN_MENU_W+title_width-1,
        MAIN_MENU_H-1,
        "  title");
    m_pTitle->SetBackColor(C_DODGER_BLUE);
    m_pTitle->SetAlignment(ALIGN_CENTER_LEFT);

    // status
    
    m_pStatus = new CTextbox(
        m_pWin,
        ID_APP_STATUS,
        width - STATUS_WIDTH,
        0,
        width-1,    // had to fix off by 1 errors in ugui for else needed -1
        MAIN_MENU_H-1,
        "status    ");
    m_pStatus->SetFont(&FONT_8X12);
    m_pStatus->SetBackColor(C_DODGER_BLUE);
    m_pStatus->SetForeColor(C_DARK_SLATE_GRAY);
    m_pStatus->SetAlignment(ALIGN_CENTER_RIGHT);
    
    // vu meter
    
    #define VU_WIDTH  139
    #define VU_TOP    6
    #define VU_HEIGHT 12
    #define VU_TWEEN  2
    #define VU_RIGHT_MARGIN  20
    
    s16 vux = width - STATUS_WIDTH - VU_WIDTH - VU_RIGHT_MARGIN;

    m_vu1 = new CVuMeter(
        m_pWin,0,ID_STATUS_VU1,
        vux,
        VU_TOP,
        vux+VU_WIDTH-1,
        VU_TOP+VU_HEIGHT-1,
        true,14);
    m_vu2 = new CVuMeter(
        m_pWin,1,ID_STATUS_VU2,
        vux,
        VU_TOP+VU_HEIGHT+VU_TWEEN,
        vux+VU_WIDTH-1,
        VU_TOP+VU_HEIGHT+VU_TWEEN+VU_HEIGHT-1,
        true,14);
}    




bool CTitlebar::Callback(UG_MESSAGE *pMsg)
{
    if (m_pMenu->Callback(pMsg))
        return true;
    
    assert(m_vu1);
    assert(m_vu2);
    
    if (m_vu1->Callback(pMsg))
        return true;
    if (m_vu2->Callback(pMsg))
        return true;
    
    if (pMsg->type  == MSG_TYPE_WINDOW)
    {
        if (pMsg->event == WIN_EVENT_UI_FRAME)
        {
            // update the status text
            
            float usPerBuffer = 1000000 / bcm_pcm.getSampleRate();
            u32 cpui = AudioStream::cpu_cycles_total;            
            u32 cpui_max = AudioStream::cpu_cycles_total_max;
            float cpu = ((float)cpui)/usPerBuffer;
            float cpu_max = ((float)cpui_max)/usPerBuffer;
            
            // ha ha ha - it's over 100%
            // may want to show "100" instead of "100.0"
            
            CString str;
            str.Format("cpu: %5.1f  max: %5.1f    \nmem:  %4d  max:  %4d    ",
                cpu,cpu_max,AudioStream::memory_used,AudioStream::memory_used_max);
            m_pStatus->SetText((const char *)str);
        }
        else if (pMsg->event == WIN_EVENT_ACTIVATE)
        {
            if (pMsg->id)   // activate controls
            {
                m_pTitle->SetBackColor(C_DODGER_BLUE);
                m_pStatus->SetBackColor(C_DODGER_BLUE);
            }
            else    // deactivate any controls
            {
                m_pTitle->SetBackColor(C_DARK_GRAY);
                m_pStatus->SetBackColor(C_DARK_GRAY);
            }
            
            m_pWin->drawImmediate(OBJ_TYPE_TEXTBOX,ID_APP_TITLE);
            m_pWin->drawImmediate(OBJ_TYPE_TEXTBOX,ID_APP_STATUS);
        }        
    }
    return false;
}


void CTitlebar::menuCallback(u8 menu_id)
{
    m_pMenu->popdown();
    m_pApp->showWindow(menu_id);
}

// static
void CTitlebar::menuCallbackStub(void *pThis, u8 menu_id)
{
    ((CTitlebar *)pThis)->menuCallback(menu_id);
}

