
#ifndef _window_record_h_
#define _window_record_h_

#include <audio/Audio.h>
#include <ugui/uguicpp.h>
#include "menu.h"
#include "track_display.h"

// RECORD_CHANNELS from audio/recorder.h

class CRecordWindow : public CWindow
{
public:
    
	CRecordWindow(CApplication *app);
	~CRecordWindow(void);

private:
    
	void Callback(UG_MESSAGE *pMsg);
    
    CApplication *m_pApp;
    CTitlebar    *m_pTitlebar;
    
    AudioRecorder *m_pRecorder;
    CRecordTrack  *m_pRecordTrack[RECORD_CHANNELS];

};


#endif  // !_window_record_h_
