#include <assert.h>
#include "track_display.h"
#include <circle/string.h>


CTrackDisplay::CTrackDisplay(
	CWindow *win,
	u8 id,
	u16 xs,
	u16 ys,
	u16 xe,
	u16 ye ) :
	CTextbox(win,id,xs,ys,xe,ye,""),
	m_pWin(win)
{
	m_area.xs = xs;
	m_area.ys = ys;
	m_area.xe = xe;
	m_area.ye = ye;
	m_buffer = 0;
	SetText("NO BUFFER!!");
	SetForeColor(C_RED);
	SetBackColor(C_BLACK);
}


void CTrackDisplay::init(
	s16 *buffer,
	u32 num_samples,
	u16 sample_rate,
	double zoom)
{
	m_buffer = buffer;
	m_fNumSamples = (double) num_samples;
	m_fSampleRate = (double) sample_rate;
	m_fZoom = zoom;
	
	m_fSampleDuration = 1.0 / m_fSampleRate;
	m_fDuration = m_fNumSamples / m_fSampleRate;
	
	onZoomChange();
}


void CTrackDisplay::setZoom(double zoom)
{
	m_fZoom = zoom;
	onZoomChange();
}


void CTrackDisplay::setStartOffset(u32 offset)
	// convert the left window position, in samples,
	// to a floating point
{
	m_fWindowLeft = ((double) offset) / m_fSampleRate;
	if (m_fWindowLeft > m_fMaxOffset)
		m_fWindowLeft = m_fMaxOffset;
}


void CTrackDisplay::setPosition(u32 position)
	// change an absolute sample number
	// into a floating point "seconds" for the cursor
	// scroll it into view as necessary
{
	m_fPosition = ((double) position) / m_fSampleRate;
	if (m_fPosition > m_fDuration)
		m_fPosition = m_fDuration;
	if (m_fPosition > m_fWindowRight ||
		m_fPosition < m_fWindowLeft)
	{
		setStartOffset(position);
		m_fPosition = m_fWindowLeft;
	}
}
    
    
void CTrackDisplay::onZoomChange()
{
	m_fWindowDuration = m_fDuration / m_fZoom;
	m_fPixelDuration  = m_fWindowDuration / ((double) (m_area.xe-m_area.xs+1));
	m_fMaxOffset      = m_fDuration - m_fWindowDuration;
	if (m_fWindowLeft > m_fMaxOffset)
		m_fWindowLeft = m_fMaxOffset;
	m_fWindowRight = m_fWindowLeft + m_fWindowDuration;
}


void CTrackDisplay::draw(bool cold)
{
	if (!m_buffer)
		return;
	
	UG_FillFrame(m_area.xs, m_area.ys, m_area.xe, m_area.ye, C_BLACK);

	// scale
	
	u16 height = m_area.ye - m_area.ys + 1;
	u16 y_zero = m_area.ys + height/2;
	UG_DrawLine(m_area.xs, y_zero, m_area.xe, y_zero, C_DARK_BLUE);
	
	s16 last_y = 0;
	double area_height = (m_area.ye-5) - (m_area.ys+5) + 1;
	
	for (double fTime = m_fWindowLeft;
				fTime < m_fWindowRight;
				fTime += m_fPixelDuration / 2.0)
	{
		u16 x = ((fTime-m_fWindowLeft) / m_fPixelDuration);
		u16 screen_x = x + m_area.xs;
		u32 sample_num = (fTime / m_fSampleDuration);
		
		assert((double)sample_num < m_fNumSamples);
		if ((double)sample_num >= m_fNumSamples)
			return;

		// Scale the s16 -32767..32767 to Area.ye-5..Area.ys+5;
		
		double value = m_buffer[sample_num];
			// -32768 .. 32767
		value = 32767.0 - value;
			// 65385 .. 0
		value /= 65535.0;
			// 1..0
		value *= area_height;
			// scaled to the area height
		value += m_area.ys + 5;
			// offset to the areay
		u16 y = value;
			// converted to an integer

		if (last_y == 0)
			last_y = y;

		UG_DrawLine(screen_x,last_y,screen_x,y,C_YELLOW);

		last_y = y;
	}
}

