
#include "window_status.h"

CStatusWindow::~CStatusWindow(void) {}

#define ID_TEST  123

CStatusWindow::CStatusWindow(CApplication *app) :
    CWindow(0,0,UG_GetXDim()-1,UG_GetYDim()-1,0),
    m_pApp(app)
{
    m_pTitlebar = new CTitlebar(this,m_pApp,1);
    new CTextbox(this,ID_TEST,300,100,450,130,"STATUS WINDOW");
}

    
void CStatusWindow::Callback(UG_MESSAGE *pMsg)
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

