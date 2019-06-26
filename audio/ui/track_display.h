#ifndef _track_display_h
#define _track_display_h

#include <ugui/uguicpp.h>


class CTrackDisplay : public CTextbox
{
public:
    
	CTrackDisplay(
        CWindow *win,
        u8  id,
        u16 xs,
        u16 ys,
        u16 xe,
        u16 ye );
    
    void init(
        s16 *buffer,
        u32 num_samples,
        u16 sample_rate,
        double zoom);
    
    void setZoom(double zoom);
    void setStartOffset(u32 offset);
    void setPosition(u32 position);
    
    void draw(bool cold);

private:
    
    CWindow *m_pWin;
    UG_AREA m_area;
    s16     *m_buffer;
    
    double   m_fZoom;
    double   m_fNumSamples;

    // set during init
    
	double	 m_fDuration;        // duration of whole buffer, in seconds
    double   m_fPosition;
	double	 m_fSampleRate;
	double	 m_fSampleDuration;
    
    // values recalculated whenever zoom changes
    
	double	 m_fWindowDuration;  // seconds being displayed
	double	 m_fPixelDuration;   // seconds per pixel
	double	 m_fWindowLeft;      // the TIME offset of the window
	double	 m_fWindowRight;     // the TIME offset of the right most pixel in the window
	double	 m_fMaxOffset;       // the largest TIME offset that the window can scroll to
    
    
    void onZoomChange();

};

#endif  // _track_display_h
