//
// timeline.cpp
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
#include "timeline.h"
#include <assert.h>

#define MAX_ZOOM	256.0
#define MIN_ZOOM	(1.0 / MAX_ZOOM)

CTimeLine::CTimeLine(unsigned nMemoryDepth, double fRuntime, unsigned nWindowSizePixel)
:	m_nMemoryDepth(nMemoryDepth),			// we are buffering 44100 samples
	m_fRuntime(fRuntime),					// over 1.0 seconds
	m_nWindowSizePixel(nWindowSizePixel),	// in a window thats 560 pixels wide
	m_fWindowLeft(0.0)
{
	assert(m_nMemoryDepth > 0);
	assert(m_fRuntime > 0.0);
	assert(m_nWindowSizePixel > 0);

	m_fSampleRate = (double) m_nMemoryDepth / m_fRuntime;
		// so, our sample rate is 44100 samples per second
	m_fMeasureDuration = 1.0 / m_fSampleRate;
		// and one sample represents 1/44100th of a second

	SetZoom(TIMELINE_ZOOM_DEFAULT);
	SetOffset(0.0);
}

CTimeLine::~CTimeLine(void)
{
}

void CTimeLine::SetZoom(int nZoom)
{
	switch (nZoom)
	{
	case TIMELINE_ZOOM_DEFAULT:
		m_fZoomFactor = 1.0;
		break;

	case TIMELINE_ZOOM_OUT:
		if (m_fZoomFactor > MIN_ZOOM)
		{
			m_fZoomFactor /= 1.1;
		}
		break;

	case TIMELINE_ZOOM_IN:
		if (m_fZoomFactor < MAX_ZOOM)
		{
			m_fZoomFactor *= 1.1;
		}
		break;

	default:
		assert(0);
		break;
	}

	// the duration of one pixel is based on the number of pixels in the window
	// and the sample rate, such that at a soom factor of 1.0, the window shows
	// the full depth of the recording (44100 samples, in our case).  At a zoom
	// factor of 2, it shows half the samples, and so on.
	
	m_fWindowDuration = m_fRuntime / m_fZoomFactor;
	m_fPixelDuration  = m_fWindowDuration / (double) m_nWindowSizePixel;
	m_fMaxOffset      = m_fRuntime - m_fWindowDuration;

	if (m_fWindowLeft > m_fMaxOffset)
	{
		m_fWindowLeft = m_fMaxOffset;
	}

	m_fWindowRight = m_fWindowLeft + m_fWindowDuration;
}

void CTimeLine::SetOffset(double fOffset)
{
	if (fOffset > m_fMaxOffset)
	{
		fOffset = m_fMaxOffset;
	}

	m_fWindowLeft = fOffset;
	m_fWindowRight = m_fWindowLeft + m_fWindowDuration;
}

void CTimeLine::AddOffset(int nPixel)
{
	m_fWindowLeft += nPixel * m_fPixelDuration;

	if (m_fWindowLeft < 0.0)
	{
		m_fWindowLeft = 0.0;
	}
	else if (m_fWindowLeft > m_fMaxOffset)
	{
		m_fWindowLeft = m_fMaxOffset;
	}

	m_fWindowRight = m_fWindowLeft + m_fWindowDuration;
}


double CTimeLine::GetRuntime(void) const
{
	return m_fRuntime;
}

double CTimeLine::GetZoomFactor(void) const
{
	return m_fZoomFactor;
}

double CTimeLine::GetSampleRate(void) const
{
	return m_fSampleRate;
}

double CTimeLine::GetWindowLeft(void) const
{
	return m_fWindowLeft;
}

double CTimeLine::GetWindowRight(void) const
{
	return m_fWindowRight;
}

double CTimeLine::GetPixelDuration(void) const
{
	return m_fPixelDuration;
}

unsigned CTimeLine::GetMeasure(double fTime) const
{
	if (fTime > m_fRuntime)
	{
		return TIMELINE_INVALID_MEASURE;
	}

	return (unsigned) (fTime / m_fMeasureDuration);
}

unsigned CTimeLine::GetPixel(double fTime) const
{
	if (   m_fWindowLeft  > fTime
	    || m_fWindowRight < fTime)
	{
		return TIMELINE_INVALID_PIXEL;
	}

	return (unsigned) ((fTime-m_fWindowLeft) / m_fPixelDuration);
}
