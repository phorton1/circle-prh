// controlwindow.h
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
// This file copied from addons/ugui/example

#ifndef _controlwindow_h
#define _controlwindow_h

#include <ugui/uguicpp.h>
#include "scopewindow.h"
// #include "recorder.h"
#include "scopeconfig.h"
#include <circle/string.h>


class CControlWindow
{
public:
    
	CControlWindow (
        UG_S16 sPosX0,
        UG_S16 sPosY0,
		CScopeWindow *pScopeWindow);    // ,
        // CScopeConfig *pConfig);
	~CControlWindow (void);

private:
	void Callback (UG_MESSAGE *pMsg);
	static void CallbackStub (UG_MESSAGE *pMsg);

	void Run (void);

	void UpdateRate (int nShift);

private:
	CScopeWindow *m_pScopeWindow;
	// CRecorder *m_pRecorder;
	// CScopeConfig *m_pConfig;

	UG_WINDOW m_Window;

	UG_TEXTBOX m_Textbox1;
	UG_TEXTBOX m_Textbox2;
	UG_TEXTBOX m_Textbox3;
	UG_TEXTBOX m_Textbox4;
	UG_TEXTBOX m_Textbox5;
	UG_TEXTBOX m_Textbox6;
	UG_TEXTBOX m_Textbox7;

	UG_BUTTON m_Button1;
	UG_BUTTON m_Button2;
	UG_BUTTON m_Button3;
	UG_BUTTON m_Button4;
	UG_BUTTON m_Button5;
	UG_BUTTON m_Button6;
	UG_BUTTON m_Button7;
	UG_BUTTON m_Button8;
	UG_BUTTON m_Button9;
	UG_BUTTON m_Button10;
	UG_BUTTON m_Button11;

	UG_CHECKBOX m_Checkbox1[4];
	UG_CHECKBOX m_Checkbox2[4];
	UG_CHECKBOX m_Checkbox3[4];

	static const unsigned s_ObjectCount = 7+11+3*4;		// must match the number of objects above
	UG_OBJECT m_ObjectList[s_ObjectCount];

	unsigned m_nChannelEnable;

	unsigned m_nParamIndex;
	CString m_Rate;

	static CControlWindow *s_pThis;
};

#endif
