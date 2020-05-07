//
// An object that combines the audio system with the ui system.
// A button representing one 'clip' in a looper


#include "awsLoopTrackButton.h"
#include <ws/wsApp.h>
#include <ws/wsEvent.h>

#include <circle/logger.h>
#define log_name  "lpt_button"

#define NUM_STATES  4

int state_colors[NUM_STATES] = {wsDARK_BLUE,wsRED,wsYELLOW,wsGREEN};


awsLoopTrackButton::awsLoopTrackButton(
		u8 row,
		u8 col,
		wsWindow *pParent,
		u16 id,
		s32 xs,
		s32 ys,
		s32 xe,
		s32 ye) :
	wsControl(pParent,id,xs,ys,xe,ye,
		WIN_STYLE_TOUCH |
		//WIN_STYLE_CLICK | 
		WIN_STYLE_3D),
	m_row(row),
	m_col(col)
{
	LOG("new awsLoopTrackButton(%d,%d)=0x%08X",row,col,(u32)this);
	// delay(50);	// grr - have to delay for the log messages to show

	m_ltb_state = 0;
	m_pressed = 0;
	
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
	
	int reg_note = 0x20 + (row * NUM_LTB_COLS) + col;
	
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

}



void awsLoopTrackButton::onUpdateTouch(bool touched)
{
	#if 1
		printf("awsLoopTrackButton(%08x)::onUpdateTouch(%d)\n",(u32)this,touched);
	#endif

	m_pressed = touched ? 1 : 0;
	
	if (touched)
	{
		m_ltb_state++;
		if (m_ltb_state >= NUM_STATES)
			m_ltb_state = 0;
	}
	
	setBit(m_state,WIN_STATE_DRAW);
}


void awsLoopTrackButton::onUpdateClick()
	// not used
{
	#if 1  // DEBUG_TOUCH
		printf("awsLoopTrackButton(%08x)::onUpdateClick()\n",(u32)this);
	#endif
	
	m_ltb_state++;
	if (m_ltb_state >= NUM_STATES)
		m_ltb_state = 0;
	
	setBit(m_state,WIN_STATE_DRAW);
	
	//	getApplication()->addEvent(new wsEvent(
	//		EVT_TYPE_BUTTON,
	//		EVENT_CLICK,
	//		this ));
}



void awsLoopTrackButton::onDraw()
{
	#if 0
		LOG("awsLoopTrackButton(%08x)::onDraw() m_state(0x%08x) m_button_state(0x%04x)",
			(u32) this,
			m_state,
			m_ltb_state);
	#endif

	wsColor bc  = state_colors[m_ltb_state];
	
	// bool pressed =
	// 	m_state & WIN_STATE_IS_TOUCHED; // ||
	//	m_button_state & BTN_STATE_PRESSED;
		
    // if (pressed)
    // {
		// if (m_button_style & BTN_STYLE_TOGGLE_COLORS)
		// {
		//    bc = m_fore_color;
		//    fc = m_back_color;
		// }
		// else if (m_button_style & BTN_STYLE_USE_ALTERNATE_COLORS )
		// {
		//   bc = m_alt_back_color;
		//   fc = m_alt_fore_color;
		// }
   //  }
	
	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
    if ( !(m_style & WIN_STYLE_TRANSPARENT))
	{
        m_pDC->fillFrame(
			m_rect_client.xs,
			m_rect_client.ys,
			m_rect_client.xe,
			m_rect_client.ye,
			bc);
	}
	
	m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);
	m_pDC->draw3DFrame(
		m_rect_abs.xs,
		m_rect_abs.ys,
		m_rect_abs.xe,
		m_rect_abs.ye,
			m_pressed ?
				buttonPressedFrameColors :
				buttonReleasedFrameColors);
}




// static
void awsLoopTrackButton::staticHandleMidiEvent(void *pObj, midiEvent *event)
{
	((awsLoopTrackButton *)pObj)->handleMidiEvent(event);
}

void awsLoopTrackButton::handleMidiEvent(midiEvent *event)
{
	s8 msg = event->getMsg();
	
	LOG("handleMidiEvent(%d,%d) cable=%d channel=%d msg=%d param1=%d param2=%d",
		m_row,m_col,
		event->getCable(),
		event->getChannel(),
		msg,
		event->getValue1(),
		event->getValue2());
	
	m_pressed = msg == 9 ? 1 : 0;
	
	if (msg == 9)	// note on
	{
		m_ltb_state++;
		if (m_ltb_state >= NUM_STATES)
			m_ltb_state = 0;
	}
	
	setBit(m_state,WIN_STATE_DRAW);
}
