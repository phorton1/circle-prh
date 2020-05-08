
#include "uiTrack.h"
#include "uiClip.h"
#include <circle/logger.h>
#define log_name  "track_ctl"

#define CLIP_OUTER_SPACE 2
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
	wsWindow(pParent,id,xs,ys,xe,ye)
		// WIN_STYLE_TRANSPARENT,
		// WIN_STYLE_TOUCH),	// |
		//WIN_STYLE_CLICK | 
		// WIN_STYLE_3D),
{
	m_selected = false;
	m_track_num = track_num;
	m_pLoopTrack = pLooper->getTrack(m_track_num);
	setForeColor(wsWHITE);
	
	// create the clip controls
	
	int width = xe-xs+1;
	int height = ye-ys+1;
	int cwidth = width - CLIP_OUTER_SPACE*2;
	int cheight = (height - CLIP_OUTER_SPACE*2 - CLIP_BUTTON_SPACE*(LOOPER_NUM_LAYERS-1)) / LOOPER_NUM_LAYERS;
	int offset = CLIP_OUTER_SPACE;
	
	LOG("width=%d height=%d cwidth=%d cheight=%d offset=%d",width,height,cwidth,cheight,offset);

	for (int i=0; i<LOOPER_NUM_LAYERS; i++)
	{
		new uiClip(
			m_track_num,
			i,
			this,
			ID_CLIP_BUTTON_BASE + i,
			CLIP_OUTER_SPACE,
			offset,
			CLIP_OUTER_SPACE + cwidth-1,
			offset + cheight-1);
		offset += cheight + CLIP_BUTTON_SPACE;
	}
}




void uiTrack::onDraw()
{
	wsColor color = m_selected ? m_fore_color : m_back_color;
	m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);

	m_pDC->drawFrame(
		m_rect_abs.xs,
		m_rect_abs.ys,
		m_rect_abs.xe,
		m_rect_abs.ye,
		color);
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