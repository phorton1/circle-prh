
#include "uiClip.h"
#include "Looper.h"
#include <circle/logger.h>
#include <circle/timer.h>
#define log_name  "ui_clip"

#define PENDING_FLASH_TIME    250000


uiClip::uiClip(
		u8 track_num,
		u8 clip_num,
		wsWindow *pParent,
		u16 id,
		s32 xs,
		s32 ys,
		s32 xe,
		s32 ye) :
	wsWindow(pParent,id,xs,ys,xe,ye,WIN_STYLE_TOUCH | WIN_STYLE_2D)
		// WIN_STYLE_CLICK
		// WIN_STYLE_3D
{
	// LOG("ctor(%d,%d) %d,%d,%d,%d",track_num,clip_num,xs,ys,xe,ye);

	m_track_num = track_num;
	m_clip_num = clip_num;

	m_pressed = 0;

	m_clip_state = 0;
	m_rec_block = 0;
	m_play_block = 0;
	m_fade_block = 0;
	m_num_blocks = 0;
	m_max_blocks = 0;
	m_selected = false;

	// m_pending_state = 0;
	m_flash = false;
	m_flash_time = 0;

	setFrameWidth(4);

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

	#if 0
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
	#endif

	setForeColor(wsWHITE);
	// setBackColor(wsBLACK);

}





// virtual
void uiClip::updateFrame()
	// detect if something has changed that should
	// cause a redraw, with some debugging output
{
	publicClip *pClip = pTheLooper->getPublicTrack(m_track_num)->getPublicClip(m_clip_num);

	bool sel 	= pClip->isSelected();
	u16 state 	= pClip->getClipState();
	u32 rec 	= pClip->getRecordBlockNum();
	u32 play 	= pClip->getPlayBlockNum();
	u32 fade 	= pClip->getCrossfadeBlockNum();
	u32 num 	= pClip->getNumBlocks();
	u32 mx 		= pClip->getMaxBlocks();

	// u16 pend = sel ? pTheLooper->getPendingCommand() : LOOP_COMMAND_NONE;

	if (sel 	!= m_selected ||
		state 	!= m_clip_state ||
		rec 	!= m_rec_block ||
		play 	!= m_play_block ||
		fade 	!= m_fade_block ||
		num 	!= m_num_blocks ||
		mx      != m_max_blocks)
	{
		m_selected = sel;
		m_clip_state = state;
		m_rec_block = rec;
		m_play_block = play;
		m_fade_block = fade;
		m_num_blocks = num;
		m_max_blocks = mx;

		setBit(m_state,WIN_STATE_DRAW);
	}

	if (sel && (state & (CLIP_STATE_PENDING_RECORD | CLIP_STATE_PENDING_PLAY)))
	{
		u32 tm = CTimer::Get()->GetClockTicks();
		if (tm > m_flash_time + PENDING_FLASH_TIME)
		{
			m_flash_time = tm;
			m_flash = !m_flash;
		}

		setBit(m_state,WIN_STATE_DRAW | WIN_STATE_REDRAW);
	}
	else if (m_flash_time)
	{
		m_flash_time = 0;
		m_flash = false;
		setBit(m_state,WIN_STATE_DRAW);
	}
}


void putInt(int x, int y, wsDC *pDC, u32 i)
{
	CString msg;
	msg.Format("%-12d",i);
	pDC->putString(x,y,(const char *)msg);
}


// virtual
void uiClip::onDraw()
{
	m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);

	wsColor frame_color = m_back_color;

	publicTrack *pTrack = pTheLooper->getPublicTrack(m_track_num);
	publicClip *pClip = pTrack->getPublicClip(m_clip_num);

	if (m_selected && pTrack->isSelected())
	{
		if (m_flash_time)
		{
			frame_color = m_flash ?
				(m_clip_state & CLIP_STATE_PENDING_RECORD ? wsRED : wsYELLOW) :
				(m_clip_state & CLIP_STATE_PENDING_RECORD ? wsWHITE : wsBLACK);
		}
		else
		{
			frame_color = m_clip_state & CLIP_STATE_RECORDED ? wsYELLOW : wsRED;
		}
	}

	m_pDC->drawFrame(
		m_rect_abs.xs,
		m_rect_abs.ys,
		m_rect_abs.xe,
		m_rect_abs.ye,
		frame_color,
		m_frame_width);


	// only draw the inside on full REDRAW

	if (!(m_state & WIN_STATE_REDRAW))
	{
		wsColor bc  = wsBLACK;
		s32 xs = m_rect_client.xs;
		s32 oxs = xs;
		s32 ys = m_rect_client.ys;
		s32 xe = m_rect_client.xe;
		s32 ye = m_rect_client.ye;

		float pct = 0;
		float width = xe - xs + 1;

		if (m_clip_state & (CLIP_STATE_RECORD_MAIN || CLIP_STATE_RECORD_MAIN))
		{
			float f_cur_block =
				m_clip_state & CLIP_STATE_RECORD_MAIN ? m_rec_block :
				m_clip_state & CLIP_STATE_PLAY_MAIN ? m_play_block : 0;

			wsColor bar_color = wsDARK_RED;
			if (m_num_blocks)	// tracks that are playing are fairly simple
			{
				bar_color = wsDARK_GREEN;
				pct = f_cur_block / m_num_blocks;
			}
			else if (m_clip_num)	// subsequent recording tracks depend on the base track length
			{
				u32 base_blocks = pTheLooper->getPublicTrack(m_track_num)->getPublicClip(0)->getNumBlocks();
				u32 num_bases = (f_cur_block / base_blocks) + 1;
				u32 use_base_blocks = num_bases * base_blocks;
				pct = f_cur_block / use_base_blocks;
			}
			else	// the zeroth clip scales in terms of 20 second units
			{
				u32 base_blocks = (10 * AUDIO_SAMPLE_RATE) / AUDIO_BLOCK_SAMPLES;
				u32 num_bases = (f_cur_block / base_blocks) + 1;
				u32 use_base_blocks = num_bases * base_blocks;
				pct = f_cur_block / use_base_blocks;
			}

			// pct = 0.10;

			// fill the bar part of the frame

			s32 pix = width * pct;
			// 	LOG("num=%d cur=%d pix=%d  width=%0.4f  pct=%0.4f",m_num_blocks,m_cur_block,pix,width,pct);
			m_pDC->fillFrame(xs,ys,xs + pix - 1,ye,bar_color);
			xs += pix;
		}
		else if (pClip->getNumBlocks())	// show recorded clips in green, empty ones in black
		{
			bc = wsDARK_GREEN;
		}

		// fill the (rest) of the farme

		m_pDC->fillFrame(xs,ys,xe,ye,bc);

		// draw the text over the bar

		m_pDC->setForeColor(m_fore_color);
		CString *stateMessage = getClipStateName(m_clip_state);
		m_pDC->putString(oxs+10,ys+5,(const char *)*stateMessage);
		delete stateMessage;

		putInt(oxs+10,ys+23,m_pDC,m_rec_block);
		putInt(oxs+10,ys+41,m_pDC,m_play_block);
		putInt(oxs+10,ys+59,m_pDC,m_fade_block);
		putInt(oxs+10,ys+77,m_pDC,m_num_blocks);
		putInt(oxs+10,ys+95,m_pDC,m_max_blocks);

		// CString msg;
		// msg.Format("%6d/%-6d %-6d",
		// 	m_rec_block,
		// 	m_num_blocks,
		// 	m_fade_block);
        //
		// m_pDC->putText(
		// 	bc,
		// 	m_fore_color,
		// 	m_rect_client,
		// 	m_align,
		// 	1,2,
		// 	(const char *)msg);
	}
}



// virtual
void uiClip::onUpdateTouch(bool touched)
{
	m_pressed = touched ? 1 : 0;

	// if (touched)
	// {
	// 	m_clip_state++;
	// 	if (m_clip_state >= NUM_STATES)
	// 		m_clip_state = 0;
	// }

	setBit(m_state,WIN_STATE_DRAW);
}




// static
void uiClip::staticHandleMidiEvent(void *pObj, midiEvent *event)
{
	((uiClip *)pObj)->handleMidiEvent(event);
}

void uiClip::handleMidiEvent(midiEvent *event)
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
		// m_clip_state++;
		// if (m_clip_state >= NUM_STATES)
		// 	m_clip_state = 0;
	}

	setBit(m_state,WIN_STATE_DRAW);
}
