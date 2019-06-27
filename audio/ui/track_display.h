#ifndef _track_display_h
#define _track_display_h

#include "vu_meter.h"
#include <ugui/uguicpp.h>
#include <circle/string.h>
#include <audio/Audio.h>


#define ID_TRACK_BASE  100
#define IDS_PER_TRACK  10
    // defines a range of ids available for the track
    

class CDialogSelectDevice : public CWindow
{
    CDialogSelectDevice(bool in);
        // select audio library streams
       
    u8 getOK()                      { return m_ok; }
    const char *getName()           { return (const char *) &m_name; }
    u8 getInstance()                { return m_instance; }
    AudioStream *getAudioStream()   { return m_pAudioStream; }
    u8 getChannel()                 { return m_channel; }
    
protected:
    
    bool        m_ok;
    bool        m_in;
    CString     m_name;
    u8          m_instance;
    AudioStream *m_pAudioStream;
    u8          m_channel;
    
};  // CDialogSelectDevice



class CButtonDeviceSelect : public CButton
{
public:
    
	CButtonDeviceSelect(
        CWindow *win,
        u8  id,
        u16 xs,
        u16 ys,
        u16 xe,
        u16 ye,
        bool out,           // select input devices for our output
        const char *name,   // default device to select
        u8 instance,        // default instance to select
        u8 channel);        // default channel to select
    
    const char *getName()           { return (const char *) &m_name; }
    u8 getInstance()                { return m_instance; }
    AudioStream *getAudioStream()   { return m_pAudioStream; }
    u8 getChannel()                 { return m_channel; }
    
   	void Callback(UG_MESSAGE *pMsg);
    
protected:
    
    bool        m_out;
    CString     m_name;
    u8          m_instance;
    AudioStream *m_pAudioStream;
    u8          m_channel;
    
};  // CButtonDeviceSelect



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
    
    bool getEnabled()   { return m_enabled; }
    
    void setEnabled(bool enabled);
    void setZoom(double zoom);
    void setStartOffset(u32 offset);
    void setPosition(u32 position);
    
    double getZoom()  { return m_fZoom; }
    
    void draw(bool cold);

private:
    
    bool    m_enabled;      // show disabled or enabled color scheme
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

};  // CTrackDisplay



class CRecordTrack : public CTextbox
{
public:
    
	CRecordTrack(
        CWindow *win,
        u8  channel_num,
        u16 xs,
        u16 ys,
        u16 xe,
        u16 ye);
    
    CTrackDisplay *getDisplay() { return m_pTrack; }
    
   	void Callback(UG_MESSAGE *pMsg);
    
protected:

    CWindow  *m_pWin;
    UG_AREA   m_area;
    u8  m_channel_num;
    AudioRecorder *m_pRecorder;

    CVuMeter            *m_pMeter;
    CTrackDisplay       *m_pTrack;
    CButtonDeviceSelect *m_pInput;
    CButtonDeviceSelect *m_pOutput;
    CButton             *m_pPlayButton;
    CButton             *m_pRecordButton;
    
    void updateButtons();
    
    
};  // CRecordTrack
    


#endif  // _track_display_h
