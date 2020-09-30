
#include "uiTrack.h"
#include "Looper.h"
#include "uiClip.h"
#include "uiWindow.h"
#include <circle/logger.h>
#include <circle/synchronize.h>
#include <system/std_kernel.h>

#define log_name  "track_ctl"

#if WITH_DEBUG_TRACK_HEADER
	#define CLIP_BUTTON_SPACE 5
	#define TRACK_MARGIN   36	// title frame for track
#else
	#define CLIP_BUTTON_SPACE 10
	#define TRACK_MARGIN   0
#endif




uiTrack::uiTrack(
		u8 track_num,
		uiWindow *pParent,
		u16 id,
		s32 xs,
		s32 ys,
		s32 xe,
		s32 ye) :
	wsWindow(pParent,id,xs,ys,xe,ye,WIN_STYLE_2D)
		// WIN_STYLE_TRANSPARENT
		// WIN_STYLE_TOUCH
		// WIN_STYLE_CLICK
{
	m_pUIWindow = pParent;

	m_track_num = track_num;
	m_selected = false;
	m_num_used = 0;
	m_num_recorded = 0;
	m_num_running = 0;

	setForeColor(wsWHITE);
    setFrameWidth(2);

	// create the clip controls

	int height = m_rect_client.ye-m_rect_client.ys+1 - TRACK_MARGIN;
	int cheight = (height - CLIP_BUTTON_SPACE*(LOOPER_NUM_LAYERS-1)) / LOOPER_NUM_LAYERS;
	int offset = TRACK_MARGIN;

	// LOG("ctor height=%d cheight=%d",height,cheight);

	for (int i=LOOPER_NUM_LAYERS-1; i>=0; i--)
	{
		int id = ID_CLIP_BUTTON_BASE + m_track_num * LOOPER_NUM_LAYERS + i;
		new uiClip(
			m_track_num,
			i,
			this,
			id,
			0,
			offset,
			m_rect_client.xe-m_rect_client.xs+1,
			offset + cheight-1);
		offset += cheight + CLIP_BUTTON_SPACE;
	}

	m_last_te_track_state = 0;
}




void uiTrack::onDraw()
{
	wsColor color = m_selected ? wsGRAY : m_back_color;
	m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);

	m_pDC->drawFrame(
		m_rect_abs.xs,
		m_rect_abs.ys,
		m_rect_abs.xe,
		m_rect_abs.ye,
		color,
		m_frame_width );

	#if WITH_DEBUG_TRACK_HEADER
		CString msg;
		msg.Format("%d/%d(%d)",m_num_running,m_num_used,m_num_recorded);
		m_pDC->putString( m_rect_client.xs+5, m_rect_client.ys+2,(const char *)msg);
	#endif
}


void uiTrack::updateFrame()
{
	publicTrack *pTrack = pTheLooper->getPublicTrack(m_track_num);

	// kludgy place for this
	// if the track state has changed,
	// send a track state change message to the TE

	int track_state = pTrack->getTrackState();
	if (track_state != m_last_te_track_state)
	{
		#if 1
			CString *ts_name = getTrackStateName(track_state);
			LOG("track(%d) state changed to 0x%04x - %s",m_track_num,track_state,(const char *)*ts_name);
			delete ts_name;
		#else
			LOG("track(%d) state changed to 0x%04x",m_track_num,track_state);
		#endif

		// send the message to the TE
		// the CC is the track number plus 0x14
		// the value is the state, which is 0..0x2f

		sendSerialMidiCC(TRACK_STATE_BASE_CC + m_track_num,track_state & 0xff);
		m_last_te_track_state = track_state;

		m_pUIWindow->enableEraseButton(m_track_num,track_state);

	}

	bool sel = pTrack->isSelected();
	u16  used = pTrack->getNumUsedClips();
	u16  rec = pTrack->getNumRecordedClips();
	u16  running = pTrack->getNumRunningClips();

	if (sel != m_selected ||
		used != m_num_used ||
		running != m_num_running ||
		rec != m_num_recorded)
	{
		m_selected = sel;
		m_num_used = used;
		m_num_recorded = rec;
		m_num_running = running;
		setBit(m_state,WIN_STATE_DRAW);
	}

	wsWindow::updateFrame();
}