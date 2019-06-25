
#include "window_record.h"

CRecordWindow::~CRecordWindow(void) {}

#define ID_TEST  123

CRecordWindow::CRecordWindow(CApplication *app) :
    CWindow(0,0,UG_GetXDim()-1,UG_GetYDim()-1,0),
    m_pApp(app)
{
    m_pTitlebar = new CTitlebar(this,m_pApp,2);
    new CTextbox(this,ID_TEST,300,100,450,130,"RECORD WINDOW");
}

    
void CRecordWindow::Callback(UG_MESSAGE *pMsg)
{
	assert(pMsg != 0);
	assert(pMsg != 0);
    if (m_pTitlebar->Callback(pMsg))
    {
        return;
    }
    
	if (pMsg->type  == MSG_TYPE_OBJECT && 
	    pMsg->id    == OBJ_TYPE_BUTTON && 
	    pMsg->event == OBJ_EVENT_PRESSED)
	{
        if (m_pTitlebar->Callback(pMsg))
            return;

		// switch (pMsg->sub_id)
		// {
        //     case ID_MENU_MAIN:
        //         m_pMainMenu->popup();
        //         break;
        // }

    }
}

