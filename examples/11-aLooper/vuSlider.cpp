
#include "vuSlider.h"
#include <circle/logger.h>
#include <utils/myUtils.h>

#define log_name "vuslider"


vuSlider::vuSlider(
	wsWindow *pParent,
	u16 id,
	s32 xs, s32 ys, s32 xe, s32 ye,
	u8 midi_channel,      // midi channel
	u8  midi_cc_num) :	  // midi cc control number) :
	wsWindow(pParent,id,xs,ys,xe,ye)
		// WIN_STYLE_TRANSPARENT)
{
	m_last_val = 0;
	
	#if 1
		midiEventHandler::registerMidiHandler(
			this,
			staticHandleMidiEvent,
			-1,		// cable
			midi_channel,	// channel 6, 0 based, as programmed on MPD21
			MIDI_EVENT_CC,	// s8 msg,
			midi_cc_num,	// 0x0D = the middle right knob on MPD218
			-1);			// any values
	#endif
}
	
			
// static
void vuSlider::staticHandleMidiEvent(void *pThis, midiEvent *event)
{
	((vuSlider *)pThis)->handleMidiEvent(event);
}

void vuSlider::handleMidiEvent(midiEvent *event)
{
	s8 val = event->getValue2();
	// LOG("val(%d)",val);
	if (val != m_last_val)
	{
		m_last_val = val;
		setBit(m_state,WIN_STATE_DRAW);
	}
}


#define BAR_HEIGHT   3

// virtual
void vuSlider::onDraw()
{
	m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);
	
    float fval = (127.0 - m_last_val);
	float height = getClientHeight() - BAR_HEIGHT;
	float pos = fval * height / 127.00;
	s32 ipos = pos;
	
	// printf("onDraw(%d) fval=%3.2f height=%3.2f pos=%3.2f ipos=%d\n",m_last_val,fval,height,pos,ipos);
	
    m_pDC->fillFrame(
		m_rect_client.xs,
		m_rect_client.ys + ipos,
		m_rect_client.xe,
		m_rect_client.ys + ipos+BAR_HEIGHT-1,wsWHITE);
}



