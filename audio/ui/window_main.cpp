
#include "window_main.h"

CMainWindow::~CMainWindow(void) {}

#define ID_TEST  123

CMainWindow::CMainWindow(CApplication *app) :
    CWindow(0,0,UG_GetXDim()-1,UG_GetYDim()-1,0),
    m_pApp(app)
{
    m_pTitlebar = new CTitlebar(this,m_pApp,0);
    new CTextbox(this,ID_TEST,5,APP_TOP_MARGIN+5,300,60,"this is some text why doesnt it show?");
    new CTextbox(this,ID_TEST+1,300,100,450,130,"MAIN WINDOW");
}

    
void CMainWindow::Callback(UG_MESSAGE *pMsg)
{
	assert(pMsg != 0);
    
    if (m_pTitlebar->Callback(pMsg))
    {
        return;
    }
    
	if (pMsg->type  == MSG_TYPE_OBJECT && 
	    pMsg->id    == OBJ_TYPE_BUTTON && 
	    pMsg->event == OBJ_EVENT_PRESSED)
	{
		// switch (pMsg->sub_id)
		// {
        //     case ID_MENU_MAIN:
        //         m_pMainMenu->popup();
        //         break;
        // }
    }
}

