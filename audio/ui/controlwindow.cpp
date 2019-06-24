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


#define TXB_VERTICAL		0
#define TXB_VERT_CHANS		1
#define TXB_HORIZONTAL		2
#define TXB_VOLUME			3
#define TXB_ACQUIRE			4
#define TXB_RATE			5

#define BTN_LEFT_QUICK		10
#define BTN_LEFT			11
#define BTN_RIGHT			12
#define BTN_RIGHT_QUICK		13
#define BTN_HOME			14
#define BTN_END				15
#define BTN_ZOOM_IN			16
#define BTN_ZOOM_OUT		17
#define BTN_RATE_LEFT		18
#define BTN_RATE_RIGHT		19
#define BTN_RUN				20

#define CHB_VERT_CH1		30
#define CHB_VERT_CH2		31
#define CHB_VERT_CH3		32
#define CHB_VERT_CH4		33

#define ID_VU_METER			40



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

	greenTextBox(this,  TXB_HORIZONTAL,     5,   100, 194, 120, 	"HORIZONTAL" );
	new CButton(this,  	BTN_LEFT_QUICK, 	10,  130, 50,  155,		"<<" );
	new CButton(this,  	BTN_LEFT, 			58,  130, 98,  155,		"<"  );
	new CButton(this,  	BTN_RIGHT, 			106, 130, 146, 155,		">"  );
	new CButton(this,  	BTN_RIGHT_QUICK, 	154, 130, 194, 155,		">>" );
	new CButton(this,  	BTN_HOME, 			10,  160, 50,  185,		"|<" );
	new CButton(this,  	BTN_END, 			58,  160, 98,  185,		">|" );
	new CButton(this,  	BTN_ZOOM_IN, 		106, 160, 146, 185,		"+"  );
	new CButton(this,  	BTN_ZOOM_OUT, 		154, 160, 194, 185,		"-"  );

	
	greenTextBox(this, 	TXB_VOLUME, 		     5,   205, 194, 225, "VOLUME" );
	#if 1	// vert 
		m_vuMeter = new CVuMeter(this,0,ID_VU_METER, 21,  240, 40,  370, 0, 12);
	#else	// horz 
		m_vuMeter = new CVuMeter(this,0,ID_VU_METER, 8,  235, 186,  255, 1, 12);
	#endif

	greenTextBox(this,  TXB_ACQUIRE, 		5, 	 385, 194, 405,		"ACQUIRE" );
	new CButton(this,  	BTN_RATE_LEFT, 		10,  415, 50,  440,		"<");
	m_tbRate = new CTextbox(this, TXB_RATE, 55,  415, 150, 440);
	m_tbRate->SetBackColor(C_LIGHT_GRAY);
	new CButton(this,  	BTN_RATE_RIGHT,     155, 415, 194, 440,		">");
	new CButton(this,  	BTN_RUN,            10,  450, 194, 474,		"RUN");

	// finished, start it up ...
	
	UpdateRate(0);
	Show();
	// m_vuMeter->Show();
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
