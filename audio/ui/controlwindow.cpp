//
// controlwindow.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <assert.h>
#include <audio/output_probe.h>
#include "controlwindow.h"
#include <circle/sched/scheduler.h>


#define TXB_VERTICAL		100
#define TXB_VERT_CHANS		101
#define TXB_HORIZONTAL		102
#define TXB_TRIGGER			103
#define TXB_TRIG_CHANS		104
#define TXB_ACQUIRE			105
#define TXB_RATE			106

#define BTN_LEFT_QUICK		200
#define BTN_LEFT			201
#define BTN_RIGHT			202
#define BTN_RIGHT_QUICK		203
#define BTN_HOME			204
#define BTN_END				205
#define BTN_ZOOM_IN			206
#define BTN_ZOOM_OUT		207
#define BTN_RATE_LEFT		208
#define BTN_RATE_RIGHT		209
#define BTN_RUN				210

#define CHB_VERT_CH1		300
#define CHB_VERT_CH2		301
#define CHB_VERT_CH3		302
#define CHB_VERT_CH4		303
#define CHB_TRIG_EN_CH1		304
#define CHB_TRIG_EN_CH2		305
#define CHB_TRIG_EN_CH3		306
#define CHB_TRIG_EN_CH4		307
#define CHB_TRIG_LV_CH1		308
#define CHB_TRIG_LV_CH2		309
#define CHB_TRIG_LV_CH3		310
#define CHB_TRIG_LV_CH4		311



void greenTextBox(CWindow *win, u8 id, s16 x, s16 y, s16 xe, s16 ye, char *text)
{
	CTextbox *box = new CTextbox(win, id, x, y, xe, ye, text);
	box->SetBackColor(C_MEDIUM_AQUA_MARINE);
	box->SetForeColor(C_WHITE);
	box->SetAlignment(ALIGN_CENTER);
}


CControlWindow::CControlWindow(UG_S16 sPosX0, UG_S16 sPosY0, CScopeWindow *pScopeWindow) :
	CWindow(sPosX0, sPosY0, sPosX0+199, sPosY0+479, WND_STYLE_2D | WND_STYLE_HIDE_TITLE),
	m_pScopeWindow(pScopeWindow),
	m_nChannelEnable(CH1|CH2),
	m_nParamIndex(0)	// an arbitrary parameter
{
	// create controls
	
	greenTextBox(this,  TXB_VERTICAL,		5,   5,   194, 25,		"VERTICAL" );
	new CTextbox(this,  TXB_VERT_CHANS, 	5,   35,  194, 55, 		"CH1   CH2   CH3   CH4" );
	for (u8 i =0; i<4; i++)
	{
		m_cbVert[i] = new CCheckbox(this, CHB_VERT_CH1+i, 17+i*48,  60,  30+i*48,  80);
		m_cbVert[i]->SetCheched(i<2);
	}

	greenTextBox(this,  TXB_HORIZONTAL,     5,   120, 194, 140, 	"HORIZONTAL" );
	new CButton(this,  	BTN_LEFT_QUICK, 	10,  150, 50,  175,		"<<" );
	new CButton(this,  	BTN_LEFT, 			58,  150, 98,  175,		"<"  );
	new CButton(this,  	BTN_RIGHT, 			106, 150, 146, 175,		">"  );
	new CButton(this,  	BTN_RIGHT_QUICK, 	154, 150, 194, 175,		">>" );
	new CButton(this,  	BTN_HOME, 			10,  180, 50,  205,		"|<" );
	new CButton(this,  	BTN_END, 			58,  180, 98,  205,		">|" );
	new CButton(this,  	BTN_ZOOM_IN, 		106, 180, 146, 205,		"+"  );
	new CButton(this,  	BTN_ZOOM_OUT, 		154, 180, 194, 205,		"-"  );

	
	greenTextBox(this, 	TXB_TRIGGER, 		5,   245, 194, 265,		"TRIGGER" );
	for (u8 i =0; i<4; i++)
	{
		new CCheckbox(this,	CHB_TRIG_EN_CH1+i, 7+i*32, 300, 37+i*32, 320);
		new CCheckbox(this,	CHB_TRIG_LV_CH1+i, 7+i*32, 325, 37+i*32, 345);
	}
	
	greenTextBox(this,  TXB_ACQUIRE, 		5, 	 385, 194, 405,		"ACQUIRE" );
	CTextbox *chans = new CTextbox(this, TXB_TRIG_CHANS, 5, 275, 194, 295, "CH1 CH2 CH3 CH4" );
	chans->SetAlignment(ALIGN_CENTER_LEFT);
	new CButton(this,  	BTN_RATE_LEFT, 		10,  415, 50,  440,		"<");
	m_tbRate = new CTextbox(this, TXB_RATE, 55,  415, 150, 440);
	m_tbRate->SetBackColor(C_LIGHT_GRAY);
	new CButton(this,  	BTN_RATE_RIGHT,     155, 415, 194, 440,		">");
	new CButton(this,  	BTN_RUN,            10,  450, 194, 474,		"RUN");

	// finished, start it up ...
	
	UpdateRate(0);
	Show();
	UG_Update();
}



void CControlWindow::Callback(UG_MESSAGE *pMsg)
{
	assert(pMsg != 0);
	if (   pMsg->type  == MSG_TYPE_OBJECT
	    && pMsg->id    == OBJ_TYPE_BUTTON
	    && pMsg->event == OBJ_EVENT_PRESSED)
	{
		assert(m_pScopeWindow != 0);

		switch (pMsg->sub_id)
		{
		case BTN_LEFT:
			m_pScopeWindow->AddChartOffset(-10);
			break;

		case BTN_RIGHT:
			m_pScopeWindow->AddChartOffset(10);
			break;

		case BTN_LEFT_QUICK:
			m_pScopeWindow->AddChartOffset(-100);
			break;

		case BTN_RIGHT_QUICK:
			m_pScopeWindow->AddChartOffset(100);
			break;

		case BTN_HOME:
			m_pScopeWindow->SetChartOffsetHome();
			break;

		case BTN_END:
			m_pScopeWindow->SetChartOffsetEnd();
			break;

		case BTN_ZOOM_IN:
			m_pScopeWindow->SetChartZoom(+1);
			break;

		case BTN_ZOOM_OUT:
			m_pScopeWindow->SetChartZoom(-1);
			break;

		case BTN_RATE_LEFT:
			UpdateRate(-1);
			return;

		case BTN_RATE_RIGHT:
			UpdateRate(1);
			return;

		case BTN_RUN:
			Run();
			break;

		default:
			return;
		}

		m_pScopeWindow->UpdateChart();
	}
}


void CControlWindow::Run(void)
{
	for (unsigned nChannel = 1; nChannel <= CHANS; nChannel++)
	{
		if (m_cbVert[nChannel-1]->GetChecked())
		{
			m_nChannelEnable |= CH(nChannel);
		}
		else
		{
			m_nChannelEnable &= ~CH(nChannel);
		}
	}

	if (m_nChannelEnable != 0)
	{
		AudioProbe *probe = AudioProbe::Get();
		printf("starting probe ...\n");
		probe->start();
		printf("probe started...\n");
		while (probe->isRunning())
		{
			CScheduler::Get()->Yield();
			CScheduler::Get()->MsSleep(100);
			printf("waiting ...\n");
		}
		m_pScopeWindow->SetChannelEnable(m_nChannelEnable);
		m_pScopeWindow->SetChartZoom(0);
		m_pScopeWindow->SetChartOffsetHome();
	}
}



void CControlWindow::UpdateRate(int nShift)
{
	switch (nShift)
	{
		case 0:
			break;
		case -1:
			if (m_nParamIndex > 0)
				m_nParamIndex--;
			break;
		case 1:
			m_nParamIndex++;
			break;
		default:
			assert(0);
			break;
	}

	m_Rate.Format("skip %u",m_nParamIndex);
	m_tbRate->SetText((char *) (const char *) m_Rate);
}
