#include <assert.h>
#include "track_display.h"
#include <circle/string.h>
#include "app.h"


//-------------------------------------------
// CDialogSelectDevice
//-------------------------------------------

#define DEVICE_DIALOG_WIDTH   400
#define DEVICE_DIALOG_HEIGHT  300
	// can only show so many in this window size


CDialogSelectDevice::CDialogSelectDevice(bool in) :
	CWindow(
		(UG_GetXDim()-DEVICE_DIALOG_WIDTH)/2,
		(UG_GetYDim()-DEVICE_DIALOG_HEIGHT)/2,
		UG_GetXDim()-(UG_GetXDim()-DEVICE_DIALOG_WIDTH)/2,
		UG_GetYDim()-(UG_GetYDim()-DEVICE_DIALOG_HEIGHT)/2,
		WND_STYLE_POPUP | WND_STYLE_3D),
	m_in(in),
	m_name("")
{
    m_ok = 0;
    m_instance = 0;
    m_pAudioStream = 0;
    m_channel = 0;
}	



//-------------------------------------------
// CButtonDeviceSelect 
//-------------------------------------------

CButtonDeviceSelect::CButtonDeviceSelect(
		CWindow *win,
		u8  id,
		u16 xs,
		u16 ys,
		u16 xe,
		u16 ye,
		bool out,           // select input devices for our output
		const char *name,   // default device to select
		u8 instance,		// default instance to select
		u8 channel) :      
	CButton(win,id,xs,ys,xe,ye,name,BTN_STYLE_3D),
	m_out(out),
	m_instance(instance),
	m_channel(channel)
{
    m_pAudioStream = 0;
	m_name.Format("%s%d[%d]",name,instance,channel);
	SetText((const char *)m_name);
}


void CButtonDeviceSelect::Callback(UG_MESSAGE *pMsg)
{
}




    


//--------------------------------------
// CTrackDisplay
//--------------------------------------

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
	double area_height = m_area.ye - m_area.ys + 1;
	
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
		value += m_area.ys;
			// offset to the areay
		u16 y = value;
			// converted to an integer

		if (last_y == 0)
			last_y = y;

		UG_DrawLine(screen_x,last_y,screen_x,y,C_YELLOW);

		last_y = y;
	}
}


//-------------------------------------------
// CRecordTrack
//-------------------------------------------

// #define ID_TRACK_BASE  100
// #define IDS_PER_TRACK  10

#define ID_TRACK_DISPLAY  		1
#define ID_TRACK_INPUT_BUTTON   2
#define ID_TRACK_OUTPUT_BUTTON  3
#define ID_TRACK_PLAY_BUTTON  	4
#define ID_TRACK_RECORD_BUTTON  5
#define ID_TRACK_VU_METER       6

#define TRACK_LEFT_OFFSET  		160
#define TRACK_RIGHT_MARGIN   	3
#define TRACK_BOTTOM_MARGIN     5

#define CONTROL_MARGIN 			3

#define VU_WIDTH  	  20
#define VU_OFFSET     TRACK_LEFT_OFFSET - VU_WIDTH - CONTROL_MARGIN

#define CONTROL_HEIGHT ((((ye-ys+1)-CONTROL_MARGIN)/3)-CONTROL_MARGIN)	//  20
#define CONTROL_WIDTH  (VU_OFFSET - CONTROL_MARGIN*4)/2


AudioStream *getFirstAudioStream()
{
	return AudioSystem::getFirstStream();
}

AudioStream *getLastAudioStream()
	// kludge for peak. need to consider how
	// topological sort treats things.
	// may just say "order you declarem is update order"
{
	AudioStream *prev = 0;
	AudioStream *obj = AudioSystem::getFirstStream();
	while (obj)
	{
		if (strcmp(obj->getName(),"peak"))
			prev = obj;
		obj = obj->getNextStream();
	}
	return prev;
}


CRecordTrack::CRecordTrack(
		CWindow *win,
		u8  channel_num,
		u16 xs,
		u16 ys,
		u16 xe,
		u16 ye	) :
	CTextbox(win,ID_TRACK_BASE + channel_num * IDS_PER_TRACK,
		xs,ys,xe,ye,""),
	m_pWin(win),
	m_channel_num(channel_num)
{
	m_area.xs = xs;
	m_area.ys = ys;
	m_area.xe = xe;
	m_area.ye = ye;

    #define MY_DARK_SLATE_BLUE  0x0842
    SetBackColor(MY_DARK_SLATE_BLUE);

    m_pRecorder = (AudioRecorder *) AudioSystem::find(0,"recorder",0);
    assert(m_pRecorder);
	
	#if 0
		printf("CRecordTrack(%d) recorder=%s:%d buffer=%08x\n",
			m_channel_num,
			m_pRecorder ? m_pRecorder->getName() : "null",
			m_pRecorder ? m_pRecorder->getInstance() : 0,
			m_pRecorder ? ((u32)m_pRecorder->getBuffer(m_channel_num)) : 0);
	#endif					
	
    m_pTrack = new CTrackDisplay(m_pWin,
		m_id + ID_TRACK_DISPLAY,
		xs + TRACK_LEFT_OFFSET,
		ys,
		xe,
		ye);
	 
	if (m_pRecorder)
	{
		m_pTrack->init(
			m_pRecorder->getBuffer(channel_num),
			RECORD_BUFFER_SAMPLES,
			RECORD_SAMPLE_RATE,
			1.00);
	}
	
	u8 srcChannel;
	AudioStream *srcDevice = m_pRecorder->getConnectedInput(channel_num, &srcChannel);
	u8 destChannel;
	AudioStream *destDevice = m_pRecorder->getFirstConnectedOutput(channel_num, &destChannel);
	
	u16 control_y = ys + CONTROL_MARGIN;
		
	m_pInput = new CButtonDeviceSelect(
		m_pWin,
		m_id + ID_TRACK_INPUT_BUTTON,
		xs + CONTROL_MARGIN,
		control_y,
		xs + VU_OFFSET - CONTROL_MARGIN*2,
		control_y + CONTROL_HEIGHT - 1,
		false,
		srcDevice ? srcDevice->getName() : "",
		srcDevice ? srcDevice->getInstance() : 0,
		srcDevice ? srcChannel : 0);

	control_y += CONTROL_HEIGHT + CONTROL_MARGIN;
	
	m_pOutput = new CButtonDeviceSelect(
		m_pWin,
		m_id +ID_TRACK_OUTPUT_BUTTON,
		xs + CONTROL_MARGIN,
		control_y,
		xs + VU_OFFSET - CONTROL_MARGIN*2,
		control_y + CONTROL_HEIGHT - 1,
		true,
		destDevice ? destDevice->getName() : "",
		destDevice ? destDevice->getInstance() : 0,
		destDevice ? destChannel : 0);
		
	control_y += CONTROL_HEIGHT + CONTROL_MARGIN;
	
	m_pPlayButton = new CButton(
		m_pWin,
		m_id + ID_TRACK_PLAY_BUTTON,
		xs + CONTROL_MARGIN,
		control_y,
		xs + CONTROL_MARGIN + CONTROL_WIDTH - 1,
		control_y + CONTROL_HEIGHT - 1,
		"play",
		BTN_STYLE_3D);
		
	m_pRecordButton = new CButton(
		m_pWin,
		m_id + ID_TRACK_RECORD_BUTTON,
		xs + CONTROL_WIDTH + CONTROL_MARGIN*2,
		control_y,
		xs + CONTROL_WIDTH*2 + CONTROL_MARGIN*2 - 1,
		control_y + CONTROL_HEIGHT - 1,
		"rec",
		BTN_STYLE_3D);
	
    m_pMeter = new CVuMeter(
		m_pWin,
		m_id + ID_TRACK_VU_METER,
		xs + VU_OFFSET + CONTROL_MARGIN,
		ys + CONTROL_MARGIN + 1,
		xs + VU_OFFSET + VU_WIDTH - CONTROL_MARGIN - 1,
		ye - CONTROL_MARGIN - 1,
		false, 9);
	
	m_pInput->SetFont(&FONT_6X10);
	m_pOutput->SetFont(&FONT_6X10);
	m_pPlayButton->SetFont(&FONT_8X12);
	m_pRecordButton->SetFont(&FONT_8X12);

	m_pInput->SetForeColor(C_DIM_GRAY);
	m_pOutput->SetForeColor(C_DIM_GRAY);
	m_pPlayButton->SetForeColor(C_DIM_GRAY);
	m_pRecordButton->SetForeColor(C_DIM_GRAY);

	m_pMeter->setAudioDevice(
		m_pRecorder->getName(),
		m_pRecorder->getInstance(),
		m_channel_num);
	
	updateButtons();
}



void CRecordTrack::updateButtons()
{
	if (m_pRecorder)
	{
		bool is_play = m_pRecorder->getPlayMask() & (1<<m_channel_num);
		bool is_record = m_pRecorder->getRecordMask() & (1<<m_channel_num);
		
		m_pRecordButton->SetBackColor(is_record ?
			C_SALMON : UGUI_STANDARD_BACK_COLOR);
		m_pRecordButton->SetForeColor(is_record ?
			C_BLACK : C_DIM_GRAY);
		m_pPlayButton->SetBackColor(is_play ?
			C_LIGHT_STEEL_BLUE : UGUI_STANDARD_BACK_COLOR);
		m_pPlayButton->SetForeColor(is_play ?
			C_BLACK : C_DIM_GRAY);
	}
}
    
	

void CRecordTrack::Callback(UG_MESSAGE *pMsg)
{
    if (pMsg->type == MSG_TYPE_WINDOW &&
		pMsg->event == WIN_EVENT_UI_FRAME)
    {
		m_pTrack->draw(true);
		m_pMeter->Callback(pMsg);
    }

	if (m_pRecorder &&
		pMsg->type  == MSG_TYPE_OBJECT && 
	    pMsg->id    == OBJ_TYPE_BUTTON && 
	    pMsg->event == OBJ_EVENT_PRESSED)
	{
		if (pMsg->sub_id == m_id + ID_TRACK_RECORD_BUTTON)
		{
			u16 mask = m_pRecorder->getRecordMask();
			if (mask & (1<<m_channel_num))
				mask &= ~(1<<m_channel_num);
			else
				mask |= 1<<m_channel_num;
			m_pRecorder->setRecordMask(mask);
			updateButtons();
		}
		else if (pMsg->sub_id == m_id + ID_TRACK_PLAY_BUTTON)
		{
			u16 mask = m_pRecorder->getPlayMask();
			if (mask & (1<<m_channel_num))
				mask &= ~(1<<m_channel_num);
			else
				mask |= 1<<m_channel_num;
			m_pRecorder->setPlayMask(mask);
			updateButtons();
		}

    }
}



