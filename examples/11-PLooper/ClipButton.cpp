
#include "ClipButton.h"
#include <circle/logger.h>
#define log_name  "clip_button"

#define NUM_STATES  4


int state_colors[NUM_STATES] = {wsBLACK,wsDARK_RED,wsBROWN,wsDARK_GREEN};


ClipButton::ClipButton(
		u8 track_num,
		u8 clip_num,
		wsWindow *pParent,
		u16 id,
		s32 xs,
		s32 ys,
		s32 xe,
		s32 ye) :
	wsWindow(pParent,id,xs,ys,xe,ye,WIN_STYLE_TOUCH)
		// WIN_STYLE_CLICK
		// WIN_STYLE_3D
{
	m_track_num = track_num;
	m_clip_num = clip_num;

	LOG("ctor(%d,%d) %d,%d,%d,%d",m_track_num,m_clip_num,xs,ys,xe,ye);
	
	LoopTrack *pTrack = pLooper->getTrack(m_track_num);
	m_pLoopClip = pTrack->getClip(m_clip_num);

	m_pressed = 0;

	m_clip_state = 0;
	m_cur_block = 0;
	m_num_blocks = 0;
	
	// registering to handle "note" events (msg==12)
	// on cable 0
	// on channel 7 (value=6)
	//
	// The MPD is setup on it's internet preset 14
	// in /documents/mpd218-preset-14-PRH2Loop.mpd218
	// which is set by pressing the prog select button
	// and the 2nd to the left pad in the top row.
	//
	// This MPD preset outputs a note on and off event for the
	// notes numbered 0x20 .. 0x2f (32 thru 47) from the top
	// left corner.
	
	int reg_note = 0x20 + (m_track_num * LOOPER_NUM_TRACKS) + m_clip_num;
	
	midiEventHandler::registerMidiHandler(
		(void *)this,
		staticHandleMidiEvent,
		-1,			// any cable
		6,			// channel = 7
		9,			// note on
		reg_note,	// inverted rows
		-1);		// ignore the last paramter

	midiEventHandler::registerMidiHandler(
		(void *)this,
		staticHandleMidiEvent,
		-1,			// any cable
		6,			// channel = 7
		8,			// note off
		reg_note,	// inverted rows
		-1);		// ignore the last paramter
	
	setForeColor(wsWHITE);
	setBackColor(wsBLACK);

}


// virtual
void ClipButton::onDraw()
{
	LOG("onDraw(%d,%d)",m_track_num,m_clip_num);
	wsColor bc  = state_colors[m_clip_state];
	m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);

	m_pDC->fillFrame(
		m_rect_client.xs,
		m_rect_client.ys,
		m_rect_client.xe,
		m_rect_client.ye,
		bc);
	
	CString msg;
	msg.Format("%8d/%-8d",
		m_cur_block,
		m_num_blocks);

	m_pDC->putText(
		bc,
		m_fore_color,
		m_rect_client,
		m_align,
		1,2,
		(const char *)msg);

}



// virtual
void ClipButton::updateFrame()
{
	u32 cur = m_pLoopClip->getCurBlock();
	u32 num = m_pLoopClip->getNumBlocks();
	
	if (cur != m_cur_block ||
		num != m_num_blocks)
	{
		m_cur_block = cur;
		m_num_blocks = num;
		setBit(m_state,WIN_STATE_DRAW);
	}
}


// virtual
void ClipButton::onUpdateTouch(bool touched)
{
	m_pressed = touched ? 1 : 0;
	
	if (touched)
	{
		m_clip_state++;
		if (m_clip_state >= NUM_STATES)
			m_clip_state = 0;
	}
	
	setBit(m_state,WIN_STATE_DRAW);
}




// static
void ClipButton::staticHandleMidiEvent(void *pObj, midiEvent *event)
{
	((ClipButton *)pObj)->handleMidiEvent(event);
}

void ClipButton::handleMidiEvent(midiEvent *event)
{
	s8 msg = event->getMsg();
	
	LOG("handleMidiEvent(%d,%d) cable=%d channel=%d msg=%d param1=%d param2=%d",
		m_track_num,m_clip_num,
		event->getCable(),
		event->getChannel(),
		msg,
		event->getValue1(),
		event->getValue2());
	
	m_pressed = msg == 9 ? 1 : 0;
	
	if (msg == 9)	// note on
	{
		m_clip_state++;
		if (m_clip_state >= NUM_STATES)
			m_clip_state = 0;
	}
	
	setBit(m_state,WIN_STATE_DRAW);
}

