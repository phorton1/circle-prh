
#ifndef _window_record_h_
#define _window_record_h_

#include <audio/Audio.h>
#include <ugui/uguicpp.h>
#include "menu.h"


class CRecordWindow : public CWindow
{
public:
    
	CRecordWindow(CApplication *app);
	~CRecordWindow(void);

private:
    
	void Callback(UG_MESSAGE *pMsg);
    
    CApplication *m_pApp;
    CTitlebar    *m_pTitlebar;

};


#endif  // !_window_record_h_
