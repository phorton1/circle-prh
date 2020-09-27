
#include "vuSlider.h"
#include "Looper.h"
#include <circle/logger.h>
#include <utils/myUtils.h>
#include <system/std_kernel.h>

#define log_name "vuslider"


#define BLANK_SIZE 1
#define BAR_HEIGHT 3
#define RED_TIME   150 		// 100's of a second, sheesh


vuSlider::vuSlider(wsWindow *pParent,u16 id, s32 xs, s32 ys, s32 xe, s32 ye,
		bool horz,
		u16 num_divs,
		s16 meter_num,
		s16 control_num,
		s16 midi_cable,
		s16 midi_channel,      // midi channel
		midiEventType_t midi_type,
		s16 midi_param1,		// midi cc control number or nrpn MSB
		s16 midi_param2) :	    // LSB
    wsWindow(pParent,id,xs,ys,xe,ye,WIN_STYLE_TOUCH | WIN_STYLE_DRAG)
{
	LOG("vuSlider(%08x,%d,%d) ctor",(u32)this,meter_num,control_num);

    m_horz = horz;
    m_num_divs = num_divs;
	m_meter_num = meter_num;
	m_control_num = control_num;

	m_last_control_value = 0;
	m_next_control_value = pTheLooper->getControlValue(m_control_num);

	for (int i=0; i<2; i++)
    {
		m_last_value[i] = 0;
		m_next_value[i] = 0;
		m_hold_red[i] = 0;
	}

	if (m_horz ? ((xe-xs-1+BLANK_SIZE) % num_divs) : ((ye-ys-1+BLANK_SIZE) % num_divs))
	{
		LOG_WARNING("major dimension+%d=%d should be evenly divisible by %d",
			BLANK_SIZE,
			m_horz ? (xe-xs-1+BLANK_SIZE) : (ye-ys-1+BLANK_SIZE),
			num_divs);
	}

	#if USE_MIDI_SYSTEM
		if (m_control_num != -1)
		{
			midiSystem::getMidiSystem()->registerMidiHandler(
				this,
				staticHandleMidiEvent,
				-1,				// cable
				midi_channel,	// channel 6, 0 based, as programmed on MPD21
				midi_type,
				midi_param1,			// 0x0D = the middle right knob on MPD218
				midi_param2);		// any values
		}
	#endif

	m_box_height = m_horz ?
		(ye-ys+1)/2 :
		((ye-ys+1) - (m_num_divs-1)*BLANK_SIZE) / m_num_divs ;
	m_box_width = m_horz ?
		((xe-xs+1) - (m_num_divs-1)*BLANK_SIZE) / m_num_divs :
		(xe-xs+1)/2;
	m_xoffset = m_horz ? m_box_width + BLANK_SIZE : 0;
	m_yoffset = m_horz ? 0 : m_box_height + BLANK_SIZE;

	m_usable_bar_area = m_horz ?
		xe-xs+1 - BAR_HEIGHT :
		ye-ys+1 - BAR_HEIGHT;
	m_bar_height = m_horz ?
		ye-ys+1 : BAR_HEIGHT;
	m_bar_width = m_horz ?
		BAR_HEIGHT : xe-xs+1;

}


// virtual
void vuSlider::updateFrame()
{
	m_next_control_value = pTheLooper->getControlValue(m_control_num);
	if (m_next_control_value != m_last_control_value)
	{
		setBit(m_state,WIN_STATE_DRAW);
	}


	if (m_meter_num < 0)	// slider only
		return;

	float peak0 = pTheLooper->getMeter(m_meter_num,0);
	float peak1 = pTheLooper->getMeter(m_meter_num,1);
	u8 value0 = (peak0 * ((float)m_num_divs) + 0.8);
	u8 value1 = (peak1 * ((float)m_num_divs) + 0.8);

	// if (value0) LOG("value0=%d",value0);

	bool clear_red = false;
	u32 red = CTimer::Get()->GetTicks();						// 100's of a second
	if (m_hold_red[0] && red>m_hold_red[0] + RED_TIME)
	{
		m_hold_red[0] = 0;
		clear_red = true;
	}
	if (m_hold_red[1] && red>m_hold_red[1] + RED_TIME)
	{
		m_hold_red[1] = 0;
		clear_red = true;
	}

	if (clear_red ||
		value0 != m_next_value[0] ||
		value1 != m_next_value[0])
	{
		m_next_value[0] = value0;
		m_next_value[1] = value1;
		setBit(m_state,clear_red?WIN_STATE_DRAW:WIN_STATE_REDRAW);
	}

}



// virtual
void vuSlider::onDraw()
{
	u16 next_control_value = m_next_control_value;
	u16 last_control_value = m_last_control_value;
	m_last_control_value = next_control_value;

	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);

	// redraw the volume control background

	if (m_control_num != -1 &&
		next_control_value != last_control_value)
	{
		// DONE IN uiWindow.cpp on the serial event
		// pTheLooper->setControl(m_control_num,next_control_value);
			// SET THE CONTROL VALUE!
			// we wait until here so that if there are multiple value events
			// in a row on a single refresh cycle, we don't send a bunch of
			// control events, but only do it once ...

		float fval = m_horz ? last_control_value : (127.0 - last_control_value);
		float pos = fval * m_usable_bar_area / 127.00;
		s32 xoff = m_horz ? pos : 0;
		s32 yoff = m_horz ? 0 : pos;

		m_pDC->fillFrame(
			m_rect_abs.xs + xoff,
			m_rect_abs.ys + yoff,
			m_rect_abs.xs + xoff + m_bar_width - 1,
			m_rect_abs.ys + yoff + m_bar_height - 1,
			m_back_color);
	}

    // We redraw the bar if the control has been invalidated
    // OR if the values have changed.

    u8 num_yellows = (((float) m_num_divs) * 0.20);

	for (int which=0; which<2; which++)
	{
		u16 val = m_next_value[which];
		u16 draw_from = m_last_value[which];
		u16 draw_to   = val;
		m_last_value[which] = val;

		if (m_state & (WIN_STATE_DRAW|WIN_STATE_INVALID))
		{
			draw_from = 0;
			draw_to = m_num_divs - 1;
		}

		if (draw_to < draw_from)
		{
			u16 t = draw_to;
			draw_to = draw_from;
			draw_from = t;
		}

		u16 use_y = m_horz ? which * m_box_height : 0;
		u16 use_x = !m_horz ? which * m_box_width : 0;
			// offset by half the width for two channels

		// LOG("draw from=%d to=%d  use x=%d y=%d",draw_from,draw_to,use_x,use_y);

		for (int i=draw_from; i<=draw_to; i++)
		{
			wsColor color = wsGREEN;
			if (m_hold_red[which] &&
				i == m_num_divs-1)
			{
				color = wsRED;
			}
			else if (i >= val)
			{
				color = wsBLACK;
			}
			else if (i == m_num_divs-1)
			{
				color = wsRED;
				m_hold_red[which] = CTimer::Get()->GetTicks();
			}
			else if (i >= m_num_divs-num_yellows-1)
			{
				color = wsYELLOW;
			}

			// note that they are in inverted order when vertical
			// so num_divs-1 becomes 0, and the last one num_divs-1
			//
			s32 inv_i = m_horz ? i : (m_num_divs-1 - i);
			s32 box_x = m_rect_abs.xs + use_x + inv_i*m_xoffset;
			s32 box_y = m_rect_abs.ys + use_y + inv_i*m_yoffset;

			m_pDC->fillFrame(
				box_x,
				box_y,
				box_x + m_box_width - 1,
				box_y + m_box_height - 1,
				color );
		}
	}


	if (m_control_num != -1)
	{
		float fval = m_horz ? next_control_value : (127.0 - next_control_value);
		float pos = fval * m_usable_bar_area / 127.00;
		s32 xoff = m_horz ? pos : 0;
		s32 yoff = m_horz ? 0 : pos;

		m_pDC->fillFrame(
			m_rect_abs.xs + xoff,
			m_rect_abs.ys + yoff,
			m_rect_abs.xs + xoff + m_bar_width - 1,
			m_rect_abs.ys + yoff + m_bar_height - 1,
			wsWHITE);
	}
}




void vuSlider::staticHandleMidiEvent(void *pThis, midiEvent *event)
{
	((vuSlider *)pThis)->handleMidiEvent(event);
}


void vuSlider::handleMidiEvent(midiEvent *event)
{
	// assumes CC or INC_DEC MIDI_EVENT_TYPEs

	s16 p1 = event->getValue1();
	s16 val = event->getValue2();

	if (p1 == MIDI_CC_DECREMENT)
		val = m_next_control_value - val;
	else if (p1 == MIDI_CC_INCREMENT)
		val = m_next_control_value + val;

	if (val <0) val = 0;
	if (val >127) val = 127;

	if (val != m_next_control_value)
	{
		m_next_control_value = val;
		setBit(m_state,WIN_STATE_DRAW);
	}
}




//------------------------------------------------------
// Dragging
//------------------------------------------------------
// These values are NOT sent to the TeensyExpression pedal !!!
// TE assumes it "owns" the volumes and is currently initialized
// to the control defaults from loopMachine.cpp.

void vuSlider::onUpdateDragMove()
{
	wsApplication *app = getApplication();
	touchState_t *touch_state = app->getTouchState();

	float pct = 1.0;
	if (m_horz)
	{
		s32 w = m_rect_abs.xe - m_rect_abs.xs;
		s32 x = touch_state->x - m_rect_abs.xs;
		if (x < 0) x = 0;
		if (x > w) x = w;
		pct = ((float)x) / ((float)w);
		LOG("DragMove: touch_x=%d xs=%d  x=%d w=%d  pct=%0.2f",
			touch_state->x,
			m_rect_abs.xs,
			x,
			w,
			pct);
	}
	else
	{
		s32 h = m_rect_abs.ye - m_rect_abs.ys + 1;
		s32 y = touch_state->y - m_rect_abs.ys;
		if (y < 0) y = 0;
		if (y > h) y = h;
		pct = 1.0 - (((float)y) / ((float)h));
		LOG("DragMove: touch_y=%d ys=%d  y=%d h=%d  pct=%0.2f",
			touch_state->y,
			m_rect_abs.ys,
			y,
			h,
			pct);
	}

	float val = pct * 127.0;
	int int_val = val;
	pTheLooper->setControl(m_control_num,int_val);
}
