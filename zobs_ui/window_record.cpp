
#include "window_record.h"
#include "app.h"
#include <circle/logger.h>

#define log_name "win_record"


#define TRACK_HEIGHT        92
#define SCALE_MARGIN        20
#define INTER_TRACK_MARGIN   5
#define RIGHT_BORDER_MARGIN  2

#define ID_BUTTON_ZOOM_OUT   10
#define ID_BUTTON_HOME       11
#define ID_BUTTON_LEFT       12
#define ID_BUTTON_STOP       13
#define ID_BUTTON_RUN        14
#define ID_BUTTON_RIGHT      15
#define ID_BUTTON_END        16
#define ID_BUTTON_ZOOM_IN    17

#define ID_FIRST_BUTTON      ID_BUTTON_ZOOM_OUT
#define NUM_BUTTONS          8

#define BUTTON_HEIGHT       34
#define BUTTON_WIDTH        80
#define BOTTOM_MARGIN       10
#define LEFT_MARGIN         10
#define RIGHT_MARGIN        10

#define BUTTON_Y        (UG_GetYDim() - BUTTON_HEIGHT - BOTTOM_MARGIN)
#define BUTTON_SPACE    ((UG_GetXDim() - LEFT_MARGIN - RIGHT_MARGIN - NUM_BUTTONS*BUTTON_WIDTH) / (NUM_BUTTONS-1))

const char *button_label[] = { "-", "<<", "<", "\xdb ", "\x10", ">", ">>", "+" };
    // for some reason the square stop button single character is not centered correctly


CRecordWindow::~CRecordWindow(void) {}


CRecordWindow::CRecordWindow(CApplication *app) :
    CWindow(0,0,UG_GetXDim()-1,UG_GetYDim()-1,0),
    m_pApp(app)
{
    // LOG("ctor",0);
    
    m_pTitlebar = new CTitlebar(this,m_pApp,2);
    m_pRecorder = (AudioRecorder *) AudioSystem::find(0,"recorder",0);
    assert(m_pRecorder);
    m_pRecorder->start();
    for (int i=0; i<RECORD_CHANNELS; i++)
    {
        m_pRecordTrack[i] = new CRecordTrack(this,i,
            0,
            APP_TOP_MARGIN + SCALE_MARGIN + TRACK_HEIGHT * i,
            UG_GetXDim() - RIGHT_BORDER_MARGIN - 1,
            APP_TOP_MARGIN + SCALE_MARGIN + TRACK_HEIGHT * (i+1) - INTER_TRACK_MARGIN);
    }

    AudioSystem::sortStreams();    
    #define MY_DARK_SLATE_BLUE  0x0842
    SetBackColor(MY_DARK_SLATE_BLUE);
    
    for (u16 i=0; i<NUM_BUTTONS; i++)
    {
        CButton *pb = new CButton(
            this,
            ID_FIRST_BUTTON + i,
            LEFT_MARGIN + i*(BUTTON_WIDTH + BUTTON_SPACE),
            BUTTON_Y,
            LEFT_MARGIN + i*(BUTTON_WIDTH + BUTTON_SPACE) + BUTTON_WIDTH-1,
            BUTTON_Y + BUTTON_HEIGHT,
            button_label[i],
            BTN_STYLE_3D);
        pb->SetBackColor(UGUI_STANDARD_BACK_COLOR); // CUGUI::Get()->getUGUI()->desktop_color);   // C_WHITE_SMOKE);
        pb->SetFont(
            i+ID_FIRST_BUTTON == ID_BUTTON_RUN ?
            &FONT_16X26 :
            &FONT_12X16);
    }
    // LOG("ctor finished",0);
}


    
void CRecordWindow::Callback(UG_MESSAGE *pMsg)
{
	assert(pMsg != 0);
	assert(pMsg != 0);

    if (m_pTitlebar->Callback(pMsg))
    {
        return;
    }
    
	if (m_pRecorder &&
        pMsg->type  == MSG_TYPE_OBJECT && 
	    pMsg->id    == OBJ_TYPE_BUTTON && 
	    pMsg->event == OBJ_EVENT_PRESSED)
	{
        if (pMsg->sub_id == ID_BUTTON_RUN)
        {
            m_pRecorder->startRecording();
        }
        else if (pMsg->sub_id == ID_BUTTON_STOP)
        {
            m_pRecorder->stopRecording();
        }
        else if (pMsg->sub_id == ID_BUTTON_ZOOM_IN ||
                 pMsg->sub_id == ID_BUTTON_ZOOM_OUT)
        {
            double zoom = m_pRecordTrack[0]->getDisplay()->getZoom();
            zoom = pMsg->sub_id == ID_BUTTON_ZOOM_IN ?
                zoom * 1.10 :
                zoom / 1.10;
            for (int i=0; i<4; i++)
                m_pRecordTrack[i]->getDisplay()->setZoom(zoom);
        }
    }

    for (int i=0; i<4; i++)
    {
        m_pRecordTrack[i]->Callback(pMsg);
    }
}

