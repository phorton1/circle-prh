
#include "window_record.h"

CRecordWindow::~CRecordWindow(void) {}

#define ID_TRACK_BASE  100

#define TRACK_LEFT_OFFSET  160
#define TRACK_TOP_MARGIN   20 + APP_TOP_MARGIN
#define TRACK_RIGHT_MARGIN   3
#define TRACK_HEIGHT        90
#define TRACK_MARGIN         5

CRecordWindow::CRecordWindow(CApplication *app) :
    CWindow(0,0,UG_GetXDim()-1,UG_GetYDim()-1,0),
    m_pApp(app)
{
    m_pTitlebar = new CTitlebar(this,m_pApp,2);
    m_pRecorder = (AudioRecorder *) AudioStream::find("recorder",0);
    assert(m_pRecorder);
    for (int i=0; i<RECORD_CHANNELS; i++)
    {
        m_pTrack[i] = new CTrackDisplay(this,
            ID_TRACK_BASE + i,
            TRACK_LEFT_OFFSET,
            TRACK_TOP_MARGIN + (TRACK_MARGIN + TRACK_HEIGHT) * i,
            UG_GetXDim() - TRACK_RIGHT_MARGIN,
            TRACK_TOP_MARGIN + (TRACK_MARGIN + TRACK_HEIGHT) * i + TRACK_HEIGHT);
        if (m_pRecorder)
        {
            m_pTrack[i]->init(
                m_pRecorder->getBuffer(i),
                RECORD_BUFFER_SAMPLES,
                RECORD_SAMPLE_RATE,
                1.00);
        }
    }
    
    #define MY_DARK_SLATE_BLUE  0x0842
    SetBackColor(MY_DARK_SLATE_BLUE); 
    SetForeColor(C_WHITE);
}


bool draw_needed = 1;

    
void CRecordWindow::Callback(UG_MESSAGE *pMsg)
{
	assert(pMsg != 0);
	assert(pMsg != 0);

    if (m_pTitlebar->Callback(pMsg))
    {
        return;
    }
    
	if (pMsg->type  == MSG_TYPE_OBJECT && 
	    pMsg->id    == OBJ_TYPE_BUTTON && 
	    pMsg->event == OBJ_EVENT_PRESSED)
	{
    }
    else if (pMsg->type == MSG_TYPE_WINDOW)
    {
        if (pMsg->event == WIN_EVENT_ACTIVATE)
        {
            draw_needed = 1;
        }
        
        else if (pMsg->event == WIN_EVENT_UI_FRAME)
        {
            if (draw_needed)
            {
                // draw_needed = 0;
                for (int i=0; i<4; i++)
                    m_pTrack[i]->draw(true);
            }
        }
    }
}

