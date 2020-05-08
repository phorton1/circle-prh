
#include "ClipButton.h"
#include <circle/logger.h>
#define log_name  "clip_button"

#define NUM_STATES  4


// int state_colors[NUM_STATES] = {wsBLACK,wsDARK_RED,wsBROWN,wsDARK_GREEN};


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
	wsColor bc  = wsBLACK;
	s32 xs = m_rect_client.xs;
	s32 ys = m_rect_client.ys;
	s32 xe = m_rect_client.xe;
	s32 ye = m_rect_client.ye;

	float pct = 0;
	float width = xe - xs + 1;
	float f_cur_block = m_cur_block;
	
	m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);

	if (m_cur_block)
	{
		wsColor bar_color = wsDARK_RED;
		if (m_num_blocks)	// tracks that are playing are fairly simple
		{
			bar_color = wsDARK_GREEN;
			pct = f_cur_block / m_num_blocks;
		}
		else if (m_clip_num)	// subsequent recording tracks depend on the base track length
		{
			u32 base_blocks = pLooper->getTrack(m_track_num)->getClip(0)->getNumBlocks();
			u32 num_bases = (m_cur_block / base_blocks) + 1;
			u32 use_base_blocks = num_bases * base_blocks;
			pct = f_cur_block / use_base_blocks;
		}
		else	// the zeroth clip scales in terms of 20 second units
		{
			u32 base_blocks = (10 * AUDIO_SAMPLE_RATE) / AUDIO_BLOCK_SAMPLES;
			u32 num_bases = (m_cur_block / base_blocks) + 1;
			u32 use_base_blocks = num_bases * base_blocks;
			pct = f_cur_block / use_base_blocks;
		}
		
		s32 pix = width * pct;
		// 	LOG("num=%d cur=%d pix=%d  width=%0.4f  pct=%0.4f",m_num_blocks,m_cur_block,pix,width,pct);
		m_pDC->fillFrame(xs,ys,xs + pix - 1,ye,bar_color);
		xs += pix;
	}

	m_pDC->fillFrame(xs,ys,xe,ye,bc);
	
		
	CString msg;
	msg.Format("%5d/%-5d",
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

