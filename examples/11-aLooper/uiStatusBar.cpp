
#include "Looper.h"
#include "uiStatusBar.h"
#include <circle/logger.h>
#include <utils/myUtils.h>


#define log_name  "ui_status"


uiStatusBar::uiStatusBar(wsWindow *pParent, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
	wsWindow(pParent,id,xs,ys,xe,ye)
{
	setBackColor(wsDARK_GREEN);
	
	m_blocks_used = 0;			
	m_blocks_free = 0;			
	m_num_tracks = 0;		
	m_num_clips = 0;
}


#include <audio/bcm_pcm.h>
u32 in_irq_count = 0;
u32 out_irq_count = 0;
u32 in_wrong_count = 0;
u32 out_wrong_count = 0;
u32 in_other_count = 0;
u32 out_other_count = 0;

// virtual
void uiStatusBar::updateFrame()
	// called 10 times per second
{
	loopBuffer *pLoopBuffer = pLooper ? pLooper->getLoopBuffer() : 0;
	if (pLooper && pLoopBuffer)
	{
		//u32 blocks_used 	= pLoopBuffer->getUsedBlocks();
		//u32 blocks_free 	= pLoopBuffer->getFreeBlocks();
		//u16 num_tracks 		= pLooper->getNumUsedTracks();
		//loopTrack *pTrack   = pLooper->getCurTrack();
		//u16 num_clips		= pTrack->getNumClips();
		
		if (
			in_irq_count    != bcm_pcm.in_irq_count ||
			out_irq_count	!= bcm_pcm.out_irq_count ||
			in_wrong_count  != bcm_pcm.in_wrong_count ||
			out_wrong_count != bcm_pcm.out_wrong_count ||
			in_other_count  != bcm_pcm.in_other_count ||
			out_other_count != bcm_pcm.out_other_count)
			// blocks_used		!= m_blocks_used	||
			// blocks_free 	!= m_blocks_free 	||
			// num_tracks 		!= m_num_tracks 	||
			// num_clips       != m_num_clips
			// )
		{
			in_irq_count    = bcm_pcm.in_irq_count;
			out_irq_count	= bcm_pcm.out_irq_count;
			in_wrong_count  = bcm_pcm.in_wrong_count;
			out_wrong_count = bcm_pcm.out_wrong_count;
			in_other_count  = bcm_pcm.in_other_count;
			out_other_count = bcm_pcm.out_other_count;
			// m_blocks_used    = blocks_used;
			// m_blocks_free 	 = blocks_free;
			// m_num_tracks 	 = num_tracks;
			// m_num_clips      = num_clips;
			setBit(m_state,WIN_STATE_DRAW);
		}
	}
	
	wsWindow::updateFrame();
}



// virtual
void uiStatusBar::onDraw()
{
	wsWindow::onDraw();
	
	CString msg;
	// "%8d/%-8d  in_other=%d out_other=%d  tracks:%d  clips:%d",
	msg.Format("%d %d %d %d %d %d",
		in_irq_count,
		out_irq_count,
		in_wrong_count,
		out_wrong_count,
		in_other_count,
		out_other_count);

		// m_blocks_used,
		// m_blocks_free,
		// m_num_tracks,	   
		// m_num_clips);

	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
	m_pDC->putText(
		m_back_color,
		m_fore_color,
		m_rect_client,
		m_align,
		1,2,
		(const char *)msg);
}


