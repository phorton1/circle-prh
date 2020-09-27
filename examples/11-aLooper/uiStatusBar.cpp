
#include "Looper.h"
#include "uiStatusBar.h"
#include <circle/logger.h>
#include <utils/myUtils.h>

#define USER_STATUS_BAR    1
	// versus old "debugging status bar"

#define log_name  "ui_status"


uiStatusBar::uiStatusBar(wsWindow *pParent, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
	wsWindow(pParent,id,xs,ys,xe,ye)
{
	#if DEBUGGING_STATUS_BAR
		setBackColor(wsDARK_GREEN);
	#endif


}


#if USER_STATUS_BAR
	s16 pct_used = -1;
#else
	#include <audio/bcm_pcm.h>
	u16 loop_running = 0;
	u32 out_irq_count = 0;
	u16 num_used_tracks = 0;
	// these counters never turned up the source of the noise
	// we merely display the out_irq_count as a general counter
	// u32 in_irq_count = 0;
	// u32 in_wrong_count = 0;
	// u32 out_wrong_count = 0;
	// u32 in_other_count = 0;
	// u32 out_other_count = 0;
#endif


// virtual
void uiStatusBar::updateFrame()
{
	#if USER_STATUS_BAR
		if (pTheLoopBuffer && pTheLooper)
		{
			float total = pTheLoopBuffer->getSizeBlocks();
			float used = pTheLoopBuffer->getUsedBlocks();

			// we have to find the recording clip, if any manually
			// and num used vs. recorded clips is used to identify it

			for (int i=0; i<LOOPER_NUM_TRACKS; i++)
			{
				publicTrack *pTrack = pTheLooper->getPublicTrack(i);
				int rec = pTrack->getNumRecordedClips();
				if (rec != pTrack->getNumUsedClips())
				{
					publicClip *pClip = pTrack->getPublicClip(rec);
					used += pClip->getRecordBlockNum() * 2;
						// it's stereo ...
				}
			}

			float val = used/total;
			val *= 100;
			s16 pct = val;
			if (pct_used != pct)
			{
				pct_used = pct;
				setBit(m_state,WIN_STATE_DRAW);
			}
		}
	#else
		if (pTheLooper)
		{
			if (loop_running 	!= pTheLoopMachine->getRunning())
			{
				loop_running    = pTheLoopMachine->getRunning();
				setBit(m_state,WIN_STATE_DRAW);
			}
			else if (out_irq_count != bcm_pcm.out_irq_count)
			{
				out_irq_count = bcm_pcm.out_irq_count;
				setBit(m_state,WIN_STATE_DRAW | WIN_STATE_REDRAW);
			}
		}
	#endif

	wsWindow::updateFrame();
}



// virtual
void uiStatusBar::onDraw()
{

	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
	m_pDC->setBackColor(m_back_color);

	#if USER_STATUS_BAR
		m_pDC->setForeColor(wsWHITE);
		m_pDC->setFont(wsFont12x16);

		CString msg1;
		msg1.Format("%d %%",pct_used);

		m_pDC->putString(
			m_clip_client.xs + 220,
			m_clip_client.ys + 10,
			(const char *)msg1);
	#else
		m_pDC->setForeColor(m_fore_color);
		if (!(m_state & WIN_STATE_REDRAW))
		{
			wsWindow::onDraw();

			CString msg1;
			msg1.Format("%d:%-12s used(%d)",
				loop_running,
				loop_running ? "RUNNING" : "",
				num_used_tracks);

			// don't need backing rect as these always grow

			m_pDC->putString(
				m_clip_client.xs + 10,
				m_clip_client.ys + 10,
				(const char *)msg1);
		}

		CString msg2;
		msg2.Format("%d",out_irq_count);
		m_pDC->putString(
			m_clip_client.xs + 300,
			m_clip_client.ys + 10,
			(const char *)msg2);
#endif
}
