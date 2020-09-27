
#include "uiClip.h"
#include "Looper.h"
#include "uiWindow.h"
#include <circle/logger.h>
#include <circle/timer.h>
#define log_name  "ui_clip"

#define PENDING_FLASH_TIME    250000

#define OLD_DEBUGGING   0


uiClip::uiClip(
		u8 track_num,
		u8 clip_num,
		wsWindow *pParent,
		u16 id,
		s32 xs,
		s32 ys,
		s32 xe,
		s32 ye) :
	wsWindow(pParent,id,xs,ys,xe,ye,
		WIN_STYLE_TOUCH |
		WIN_STYLE_2D |
		WIN_STYLE_DRAG |
		WIN_STYLE_CLICK )
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

	m_draw_text	= true;
	m_mute = 0;
	m_volume = 0;

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
	setBackColor(wsBLACK);

	setBit(m_state, WIN_STATE_DRAW | WIN_STATE_REDRAW);
}





// virtual
void uiClip::updateFrame()
	// detect if something has changed that should
	// cause a redraw, with some debugging output
{
	publicTrack *pTrack = pTheLooper->getPublicTrack(m_track_num);
	publicClip *pClip = pTrack->getPublicClip(m_clip_num);

	bool sel 	= pTrack->isSelected();
	u16 state 	= pClip->getClipState();
	u32 rec 	= pClip->getRecordBlockNum();
	u32 play 	= pClip->getPlayBlockNum();
	u32 fade 	= pClip->getCrossfadeBlockNum();
	u32 num 	= pClip->getNumBlocks();
	u32 mx 		= pClip->getMaxBlocks();

	if (sel 	!= m_selected ||
		state 	!= m_clip_state ||
		rec 	!= m_rec_block ||
		play 	!= m_play_block ||
		fade 	!= m_fade_block ||
		num 	!= m_num_blocks ||
		mx      != m_max_blocks)
	{
		if (state != m_clip_state)
			m_draw_text	= true;

		m_selected = sel;
		m_clip_state = state;
		m_rec_block = rec;
		m_play_block = play;
		m_fade_block = fade;
		m_num_blocks = num;
		m_max_blocks = mx;
		setBit(m_state, WIN_STATE_DRAW | WIN_STATE_REDRAW);
	}

	bool mute   = pClip->isMuted();
	int vol     = pClip->getVolume();
	if (mute != m_mute ||
		vol != m_volume)
	{
		setBit(m_state, WIN_STATE_DRAW | WIN_STATE_REDRAW);
		m_draw_text	= true;

		if (m_mute != mute)
		{
			int cc_num = CLIP_MUTE_BASE_CC +
				m_track_num * LOOPER_NUM_LAYERS + m_clip_num;
			sendSerialMidiCC(cc_num,mute);
		}
		m_mute = mute;
		m_volume = vol;
	}

	u16 pend = pTheLooper->getPendingCommand();
	if (sel && pend)
	{
		u32 tm = CTimer::Get()->GetClockTicks();
		if (tm > m_flash_time + PENDING_FLASH_TIME)
		{
			m_flash_time = tm;
			m_flash = !m_flash;
		}
		setBit(m_state,WIN_STATE_DRAW);
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
	u16 pend = pTheLooper->getPendingCommand();

	if (m_selected)
	{
		bool playing = m_clip_state & CLIP_STATE_PLAY_MAIN;
		bool recording = m_clip_state & (CLIP_STATE_RECORD_IN | CLIP_STATE_RECORD_MAIN);
		bool recorded = m_clip_state & CLIP_STATE_RECORDED;
		bool will_play = recording || recorded;

		if (pend == LOOP_COMMAND_STOP)
		{
			if (playing || recording)
				frame_color = wsWHITE;
		}
		else if (pend == LOOP_COMMAND_RECORD)
		{
			if (m_clip_num == pTrack->getNumUsedClips())
			    frame_color = wsRED;
			else if (will_play)
			    frame_color = wsYELLOW;
		}
		else if (pend == LOOP_COMMAND_PLAY)
		{
			if (will_play)
			    frame_color = wsYELLOW;
		}
		else if (m_clip_state & CLIP_STATE_PLAY_MAIN)
		{
			frame_color = wsYELLOW;
		}
		else if (m_clip_state & (CLIP_STATE_RECORD_IN | CLIP_STATE_RECORD_MAIN))
		{
			frame_color = wsRED;
		}

		if (m_flash_time && !m_flash)
			frame_color = wsBLACK;
	}

	m_pDC->drawFrame(
		m_rect_abs.xs,
		m_rect_abs.ys,
		m_rect_abs.xe,
		m_rect_abs.ye,
		frame_color,
		m_frame_width);



	#define BAR_HEIGHT  40

	s32 xs = m_rect_client.xs;
	s32 ys = m_rect_client.ys;
	s32 xe = m_rect_client.xe;
	s32 ye = m_rect_client.ye;

	// only draw the text if necessary

	if (m_draw_text)
	{
		m_draw_text = false;

		s32 text_area_ys = ys + BAR_HEIGHT;
		m_pDC->fillFrame(xs,text_area_ys,xe,ye,m_back_color);

		int color = m_clip_state ? wsWHITE : wsMEDIUM_PURPLE;	// wsGRAY;
		m_pDC->setFont(wsFont12x16);
		m_pDC->setForeColor(color);
		m_pDC->setBackColor(m_back_color);

		int y = ye - 45;
		m_pDC->putString(xs+5,y,m_mute ? "MUTE" : "    ");

		y += 20;
		CString msg;
		msg.Format("%-3d",m_volume);
		m_pDC->putString(xs+5,y,(const char *)msg);
	}

	// only draw the bar if it moved

	if (m_state & WIN_STATE_REDRAW)
	{
		wsColor bc  = wsBLACK;
		s32 bar_ye = ys + BAR_HEIGHT - 1;
		#if OLD_DEBUGGING
			s32 oxs = xs;
		#endif

		float pct = 0;
		float width = xe - xs + 1;

		if (m_clip_state & (CLIP_STATE_PLAY_MAIN | CLIP_STATE_RECORD_MAIN | CLIP_STATE_RECORD_IN))
		{
			float f_cur_block =
				m_clip_state & (CLIP_STATE_RECORD_MAIN | CLIP_STATE_RECORD_IN) ? m_rec_block :
				m_clip_state & CLIP_STATE_PLAY_MAIN ? m_play_block : 0;

			wsColor bar_color = m_mute ? wsDARK_RED : wsRED;
			if (m_num_blocks)	// tracks that are playing are fairly simple
			{
				bar_color = m_mute ? wsGRAY : wsYELLOW;	// wsDARK_GREEN;
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

			// fill the bar with color

			s32 pix = width * pct;
			// 	LOG("num=%d cur=%d pix=%d  width=%0.4f  pct=%0.4f",m_num_blocks,m_cur_block,pix,width,pct);
			m_pDC->fillFrame(xs,ys,xs + pix - 1,bar_ye,bar_color);
			xs += pix;
		}
		else if (pClip->getNumBlocks())	// show recorded clips in green, empty ones in black
		{
			bc = m_mute ? wsGRAY : wsGREEN;
		}

		// fill the (rest) of the bar area ....

		m_pDC->fillFrame(xs,ys,xe,bar_ye,bc);

		// old debugging

		#if OLD_DEBUGGING
			// draw the text over the bar

			m_pDC->setForeColor(m_fore_color);
			CString *stateMessage = getClipStateName(m_clip_state);
			m_pDC->putString(oxs+5,ys+2,(const char *)*stateMessage);
			delete stateMessage;

			putInt(oxs+5,ys+20,m_pDC,m_rec_block);
			putInt(oxs+5,ys+36,m_pDC,m_play_block);
			putInt(oxs+5,ys+52,m_pDC,m_fade_block);
			putInt(oxs+5,ys+68,m_pDC,m_num_blocks);
			putInt(oxs+5,ys+84,m_pDC,m_max_blocks);

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
		#endif

	}		// WIN_STATE_REDRAW == update the moving bar and changes only
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




void uiClip::onUpdateDragMove()
{
	wsApplication *app = getApplication();
	touchState_t *touch_state = app->getTouchState();
	s32 h = m_rect_abs.ye - m_rect_abs.ys + 1;
	s32 y = touch_state->y - m_rect_abs.ys;
	if (y < 0) y = 0;
	if (y > h) y = h;
	float pct = 1.0 - (((float)y) / ((float)h));
	LOG("clip DragMove: touch_y=%d ys=%d  y=%d h=%d  pct=%0.2f",
		touch_state->y,
		m_rect_abs.ys,
		y,
		h,
		pct);

	float val = pct * 127.0;
	int int_val = val;

	publicTrack *pTrack = pTheLooper->getPublicTrack(m_track_num);
	publicClip *pClip = pTrack->getPublicClip(m_clip_num);
	pClip->setVolume(int_val);
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
