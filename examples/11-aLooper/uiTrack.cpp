
#include "uiTrack.h"
#include "uiClip.h"
#include <circle/logger.h>
#define log_name  "track_ctl"

#define CLIP_BUTTON_SPACE 10

#define ID_CLIP_BUTTON_BASE 500


uiTrack::uiTrack(
		u8 track_num,
		wsWindow *pParent,
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
	m_selected = false;
	m_track_num = track_num;
	m_pLoopTrack = pLooper->getTrack(m_track_num);
	setForeColor(wsWHITE);
    setFrameWidth(2);
	
	// create the clip controls
	
	int height = m_rect_client.ye-m_rect_client.ys+1;
	int cheight = (height - CLIP_BUTTON_SPACE*(LOOPER_NUM_LAYERS-1)) / LOOPER_NUM_LAYERS;
	int offset = 0;
	
	// LOG("ctor height=%d cheight=%d",height,cheight);

	for (int i=0; i<LOOPER_NUM_LAYERS; i++)
	{
		new uiClip(
			m_track_num,
			i,
			this,
			ID_CLIP_BUTTON_BASE + i,
			0,
			offset,
			m_rect_client.xe-m_rect_client.xs+1,
			offset + cheight-1);
		offset += cheight + CLIP_BUTTON_SPACE;
	}
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
}


void uiTrack::updateFrame()
{
	bool sel = m_pLoopTrack->isSelected();
	if (sel != m_selected)
	{
		m_selected = sel;
		setBit(m_state,WIN_STATE_DRAW);
	}
	
	wsWindow::updateFrame();
}