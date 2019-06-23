//
// scopewindow.cpp
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

#include <audio/output_probe.h>
#include "scopewindow.h"
#include <circle/string.h>
#include <assert.h>



#define WIDTH		600
#define HEIGHT		480
#define MARGIN		5


CScopeWindow::CScopeWindow(UG_S16 sPosX0, UG_S16 sPosY0) :
	CWindow(sPosX0, sPosY0, sPosX0+WIDTH-1, sPosY0+HEIGHT-1, WND_STYLE_2D | WND_STYLE_HIDE_TITLE),
	m_nChannelEnable(0),
	m_pTimeLine(0)
{
	m_textbox = new CTextbox(this,TXB_ID_0, 5, 5, 149, 25, "AudioScope");
	m_textbox->SetFont(&FONT_10X16);
	m_textbox->SetBackColor(C_LIGHT_GRAY);
	m_textbox->SetForeColor(C_BLACK);
	m_textbox->SetAlignment(ALIGN_CENTER);

	Show();

	UG_Update();

	UpdateChart();
}


CScopeWindow::~CScopeWindow(void)
{
	delete m_pTimeLine;
	m_pTimeLine = 0;
}


void CScopeWindow::UpdateChart(void)
{
	UG_AREA Area;
	GetArea(&Area);
	
	Area.xs += MARGIN;
	Area.xe -= MARGIN;
	Area.ys += MARGIN+20+10;	// consider title height and margin below
	Area.ye -= MARGIN;

	UG_FillFrame(Area.xs, Area.ys, Area.xe, Area.ye, C_BLACK);

	if (m_pTimeLine == 0)
	{
		return;
	}

	// display headline

	double fZoomFactor = m_pTimeLine->GetZoomFactor();
	double fSampleRate = m_pTimeLine->GetSampleRate();
	
	CString String;
	String.Format("%.1fKHz    zoom:%.3fx",
		fSampleRate/1000.0,
		fZoomFactor);

	UG_SetBackcolor(C_BLACK);
	UG_SetForecolor(C_WHITE);
	UG_FontSelect(&FONT_6X8);
	UG_PutString(Area.xs+5, Area.ys+1, (char *) (const char *) String);

	Area.ys += 12;
	UG_DrawLine(Area.xs, Area.ys, Area.xe, Area.ys, C_WHITE);

	// count active channels

	unsigned nChannelCount = 0;
	for (unsigned nChannel = 1; nChannel <= CHANS; nChannel++)
	{
		if (m_nChannelEnable & CH(nChannel))
		{
			nChannelCount++;
		}
	}

	if (nChannelCount == 0)
	{
		return;
	}

	if (nChannelCount == 1)
	{
		Area.ys += 100;
		Area.ye -= 100;
	}

	// draw channels

	UG_S16 sChannelHeight = (Area.ye-Area.ys+1) / nChannelCount;

	unsigned nChannelsDrawn = 0;
	for (unsigned nChannel = 1; nChannel <= CHANS; nChannel++)
	{
		if (m_nChannelEnable & CH(nChannel))
		{
			UG_AREA ChannelArea;
			ChannelArea.xs = Area.xs;
			ChannelArea.xe = Area.xe;
			ChannelArea.ys = Area.ys + sChannelHeight*nChannelsDrawn;
			ChannelArea.ye = ChannelArea.ys + sChannelHeight-1;

			DrawScale(ChannelArea);
			
			DrawChannel(nChannel, ChannelArea);

			nChannelsDrawn++;
		}
	}
}


void CScopeWindow::SetChannelEnable(unsigned nMask)
{
	m_nChannelEnable = nMask;
	delete m_pTimeLine;
	m_pTimeLine = new CTimeLine(
		44100,			// sample_depth
		1.0,			// seconds
		WIDTH-2*MARGIN);
}


void CScopeWindow::SetChartZoom(int nZoom)
{
	if (m_pTimeLine != 0)
	{
		switch (nZoom)
		{
		case 0:
			m_pTimeLine->SetZoom(TIMELINE_ZOOM_DEFAULT);
			break;

		case -1:
			m_pTimeLine->SetZoom(TIMELINE_ZOOM_OUT);
			break;

		case +1:
			m_pTimeLine->SetZoom(TIMELINE_ZOOM_IN);
			break;

		default:
			assert(0);
			break;
		}
	}
}

void CScopeWindow::SetChartOffsetHome(void)
{
	if (m_pTimeLine != 0)
	{
		m_pTimeLine->SetOffset(0.0);
	}
}

void CScopeWindow::SetChartOffsetEnd(void)
{
	if (m_pTimeLine != 0)
	{
		m_pTimeLine->SetOffset(TIMELINE_MAX_OFFSET);
	}
}

void CScopeWindow::AddChartOffset(int nPixel)
{
	if (m_pTimeLine != 0)
	{
		m_pTimeLine->AddOffset(nPixel);
	}
}

void CScopeWindow::DrawScale(UG_AREA &Area)
{
	u16 height = Area.ye - Area.ys + 1;
	u16 y_zero = Area.ys + height/2;
	UG_DrawLine(Area.xs, y_zero, Area.xe, y_zero, C_DARK_BLUE);
}


void CScopeWindow::DrawChannel(unsigned nChannel, UG_AREA &Area)
{
	assert(m_pTimeLine != 0);
	// printf("DrawChannel(%d,%d,%d)\n",nChannel,Area.ys,Area.ye);

	UG_DrawLine(Area.xs, Area.ys+5, Area.xe, Area.ys+5, C_PURPLE);
	UG_DrawLine(Area.xs, Area.ye-5, Area.xe, Area.ye-5, C_PURPLE);
	
	static const UG_COLOR Colors[] = {C_YELLOW, C_CYAN, C_MAGENTA, C_ORANGE};
	UG_COLOR Color = Colors[(nChannel-1) & 3];

	UG_S16 nLastPosY = 0;
	double area_height = (Area.ye-5) - (Area.ys+5) + 1;
	
	for (double fTime = m_pTimeLine->GetWindowLeft();
				fTime < m_pTimeLine->GetWindowRight();
				fTime += m_pTimeLine->GetPixelDuration() / 2.0)
	{
		unsigned nPixel = m_pTimeLine->GetPixel(fTime);
		assert(nPixel != TIMELINE_INVALID_PIXEL);
		UG_S16 sPosX = Area.xs + nPixel;

		unsigned nMeasure = m_pTimeLine->GetMeasure(fTime);
		assert(nMeasure != TIMELINE_INVALID_MEASURE);

		// Scale the s16 -32767..32767 to Area.ye-5..Area.ys+5;
		
		double value = AudioProbe::m_buffer[nChannel-1][nMeasure];
			// -32768 .. 32767
		value = 32767.0 - value;
			// 65385 .. 0
		value /= 65535.0;
			// 1..0
		value *= area_height;
			// scaled to the area height
		value += Area.ys + 5;
			// offset to the areay
		u16 nPosY = value;
			// converted to an integer

		if (nLastPosY == 0)
		{
			nLastPosY = nPosY;
		}

		UG_DrawLine(sPosX, nLastPosY, sPosX, nPosY, Color);

		nLastPosY = nPosY;
	}

	UG_FontSelect(&FONT_8X14);
	UG_PutChar('0'+nChannel, Area.xs+5, Area.ys+(Area.ye-Area.ys+1-14)/2, C_BLACK, Color);
}

