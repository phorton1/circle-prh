// controlwindow.h
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
// This file copied from addons/ugui/example

#ifndef _controlwindow_h
#define _controlwindow_h

#include <ugui/uguicpp.h>
#include "scopewindow.h"
#include <circle/string.h>


class CControlWindow : public CWindow
{
public:
    
	CControlWindow(
        UG_S16 sPosX0,
        UG_S16 sPosY0,
		CScopeWindow *pScopeWindow);   

private:
    
	void Callback(UG_MESSAGE *pMsg);

	void Run(void);

	void UpdateRate(int nShift);

private:

	CScopeWindow *m_pScopeWindow;
    
    CTextbox    *m_tbRate;
    CCheckbox   *m_cbVert[4];

	unsigned    m_nChannelEnable;
	unsigned    m_nParamIndex;
	CString     m_Rate;
};

#endif
