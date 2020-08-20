
#include "Looper.h"
#include "uiStatusBar.h"
#include <circle/logger.h>
#include <utils/myUtils.h>


#define log_name  "ui_status"


uiStatusBar::uiStatusBar(wsWindow *pParent, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
	wsWindow(pParent,id,xs,ys,xe,ye)
{
	setBackColor(wsDARK_GREEN);

}


#include <audio/bcm_pcm.h>

u16 loop_state = 99;   // some bogus value
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
	if (pTheLooper)	// && pTheLoopBuffer)
	{
		if (loop_state      != pTheLoopMachine->getLoopState()
			#if DEBUG_UPDATE
				|| in_irq_count    != bcm_pcm.in_irq_count
				|| out_irq_count	!= bcm_pcm.out_irq_count
				|| in_wrong_count  != bcm_pcm.in_wrong_count
				|| out_wrong_count != bcm_pcm.out_wrong_count
				|| in_other_count  != bcm_pcm.in_other_count
				|| out_other_count != bcm_pcm.out_other_count
			#endif
			)
		{
			setBit(m_state,WIN_STATE_DRAW);
		}
	}

	wsWindow::updateFrame();
}



// virtual
void uiStatusBar::onDraw()
{
	wsWindow::onDraw();

	loop_state      = pTheLoopMachine->getLoopState();
	out_irq_count	= bcm_pcm.out_irq_count;
	in_wrong_count  = bcm_pcm.in_wrong_count;
	out_wrong_count = bcm_pcm.out_wrong_count;
	in_other_count  = bcm_pcm.in_other_count;
	out_other_count = bcm_pcm.out_other_count;

	CString msg;
	msg.Format("%-12s  %d %d %d %d %d %d",
		getLoopStateName(loop_state),
		in_irq_count,
		out_irq_count,
		in_wrong_count,
		out_wrong_count,
		in_other_count,
		out_other_count);

	m_pDC->setForeColor(m_fore_color);
	m_pDC->setBackColor(m_back_color);
	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);

	// don't need backing rect as these always grow

	m_pDC->putString(
		m_clip_client.xs + 10,
		m_clip_client.ys + 10,
		(const char *)msg);

}
