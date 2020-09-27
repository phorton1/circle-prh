
#include "uiWindow.h"
#include <circle/logger.h>
#include <utils/myUtils.h>
#include "Looper.h"
#include "uiStatusBar.h"
#include "uiTrack.h"
#include "vuSlider.h"
#include <system/std_kernel.h>
	// to get to serial port


#define log_name  "loopwin"

#define USE_SERIAL_INTERRUPTS 1
	// There was a lag when polling the serial port, so I (finally)
	// went into circle and implemented a decent serial port interrupt
	// API.  If this is set to 1, the interrupt handler is used, if
	// not, polling is used.


#define TOP_MARGIN 		32
#define BOTTOM_MARGIN  	60
#define LEFT_MARGIN    	150

#define VU_TOP  	TOP_MARGIN+40
#define VU_BOTTOM   VU_TOP + 180

#define TRACK_VSPACE  	5
#define TRACK_HSPACE  	5

#define ID_LOOP_STATUS_BAR     101
#define ID_VU1                201
#define ID_VU2                202
#define ID_VU3                203
#define ID_VU4                204

#define ID_VU_SLIDER           250

#define ID_TRACK_CONTROL_BASE  300

#define ID_LOOP_STOP_BUTTON         401
#define ID_LOOP_DUB_BUTTON    		401
#define ID_LOOP_TRACK_BUTTON_BASE   410


//----------------------------------------------------------------

#if 0
u16 uiWindow::getButtonFunction(u16 num)
{
	if (!pTheLooper)
		return LOOP_COMMAND_NONE;

	if (num == 0)
	{
		if (pTheLooper->canDo(LOOP_COMMAND_RECORD))
			return LOOP_COMMAND_RECORD;
		if (pTheLooper->canDo(LOOP_COMMAND_PLAY))
			return LOOP_COMMAND_PLAY;
	}
	if (num == 1)
	{
		if (pTheLooper->canDo(LOOP_COMMAND_STOP))
			return LOOP_COMMAND_STOP;
	}
	if (num == 2)
	{
		if (pTheLooper->canDo(LOOP_COMMAND_SELECT_NEXT_CLIP))
			return LOOP_COMMAND_SELECT_NEXT_CLIP;
	}
	if (num == 3)
	{
		if (pTheLooper->canDo(LOOP_COMMAND_SELECT_NEXT_TRACK))
			return LOOP_COMMAND_SELECT_NEXT_TRACK;
	}
	if (num == 4)
	{
		if (pTheLooper->canDo(LOOP_COMMAND_STOP_IMMEDIATE))
			return LOOP_COMMAND_STOP_IMMEDIATE;
		if (pTheLooper->canDo(LOOP_COMMAND_CLEAR_ALL))
			return LOOP_COMMAND_CLEAR_ALL;
	}
	return LOOP_COMMAND_NONE;
}
#endif



//----------------------------------------------------------------

uiWindow::uiWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
	wsTopLevelWindow(pApp,id,xs,ys,xe,ye)
{
	LOG("uiWindow ctor",0);

	last_running = 0;

	setBackColor(wsDARK_BLUE);

	int height = ye-ys+1;
    int width = xe-xs+1;

	new uiStatusBar(this,ID_LOOP_STATUS_BAR,165,0,width-165,TOP_MARGIN-1);
	LOG("status bar created",0);

	// TOP LEFT INPUT VU METER - CODEC audio input, period

	// mpd knobs, double numbers are inc_dec msb lsb's
	// ith increments of two
	//
	//   in:DEC2(0x0A)		  		out:DEC2(0z0B)
	//   unuaed:CC(0x0C)      		mix:DEC1(0x0D:0x00) scale=2
	//   thru:DEC1(0x0E,0x00)=2     loop:CC(0x0F)

	#if WITH_METERS
		new vuSlider(this,ID_VU2, 6, 2, 150, 28, true, 12,
			METER_INPUT,
			CONTROL_INPUT_GAIN,
			-1,							// cable
			-1,							// 0 based channel
			MIDI_EVENT_TYPE_INC_DEC2,	// slow continuous
			0x0A);						// LSB
	#endif


	// TOP RIGHT OUTPUT VU METER - output gain, no monitor

	#if WITH_METERS
		new vuSlider(this,ID_VU2, width-150, 2, width-6, 28, true, 12,
			-1,		 					// no meter
			CONTROL_OUTPUT_GAIN,
			-1,							// cable
			-1,							// 0 based channel
			MIDI_EVENT_TYPE_INC_DEC2,	// slow continuous
			0x0B);						// top right knob
	#endif

	// LEFT - THRU vu

	#if WITH_METERS
		wsStaticText *pt1 = new wsStaticText(this,0,"THRU",6,TOP_MARGIN+23,42,TOP_MARGIN+39);
		pt1->setAlign(ALIGN_CENTER);
		pt1->setForeColor(wsWHITE);
		pt1->setFont(wsFont7x12);

		new vuSlider(this,ID_VU2, 8, VU_TOP, 40, VU_BOTTOM, false, 12,
			METER_THRU,
			CONTROL_THRU_VOLUME,
			-1,		// cable
			-1,		// 0 based channel
			MIDI_EVENT_TYPE_INC_DEC1,	// faster continous
			0x0E,						// bottom left knob
			0x00);
	#endif

	// MIDDLE - LOOP vu

	#if WITH_METERS
		wsStaticText *pt2 = new wsStaticText(this,0,"LOOP",50,TOP_MARGIN+23,86,TOP_MARGIN+39);
		pt2->setAlign(ALIGN_CENTER);
		pt2->setForeColor(wsWHITE);
		pt2->setFont(wsFont7x12);

		new vuSlider(this,ID_VU3, 52, VU_TOP, 84, VU_BOTTOM, false, 12,
			METER_LOOP,
			CONTROL_LOOP_VOLUME,
			-1,		// cable
			-1,		// 0 based channel
			MIDI_EVENT_TYPE_CC,			// jumpy CC for foot pedal
			0x0F);						// bottom right knob
	#endif

	// RIGHT - MIX vu

	#if WITH_METERS
		wsStaticText *pt3 = new wsStaticText(this,0,"MIX",94,TOP_MARGIN+23,130,TOP_MARGIN+39);
		pt3->setAlign(ALIGN_CENTER);
		pt3->setForeColor(wsWHITE);
		pt3->setFont(wsFont7x12);

		new vuSlider(this,ID_VU4, 96, VU_TOP, 128, VU_BOTTOM, false, 12,
			METER_MIX,
			CONTROL_MIX_VOLUME,
			-1,		// cable
			-1,		// 0 based channel
			MIDI_EVENT_TYPE_INC_DEC1,	// faster continous
			0x0D,						// right knob
			0x00);
	#endif



	// UI TRACKS
	// and TRACK buttons

	LOG("creating ui_tracks and track buttons",0);

	int btop = height-BOTTOM_MARGIN;
	int cheight = height-TOP_MARGIN-BOTTOM_MARGIN-TRACK_VSPACE*2;
	int cwidth = (width-LEFT_MARGIN-1-TRACK_HSPACE*(LOOPER_NUM_TRACKS+1)) / LOOPER_NUM_TRACKS;
	int step = LEFT_MARGIN + TRACK_HSPACE;
	for (int i=0; i<LOOPER_NUM_TRACKS; i++)
	{
		LOG("creating ui_track(%d)",i);
		new uiTrack(
			i,
			this,
			ID_TRACK_CONTROL_BASE + i,
			step,
			TOP_MARGIN + TRACK_VSPACE,
			step + cwidth -1,
			TOP_MARGIN + TRACK_VSPACE + cheight - 1);


		pTrackButtons[i] = new

			#if USE_MIDI_SYSTEM
				wsMidiButton(
			#else
				wsButton(
			#endif
				this,
				ID_LOOP_TRACK_BUTTON_BASE + i,
				getLoopCommandName(LOOP_COMMAND_TRACK_BASE + i),
				step+5,
				btop+5,
				step + cwidth - 5,
				btop+49);

		step += cwidth + TRACK_HSPACE;
		LOG("finished creating ui_track(%d)",i);
	}

	pStopButton = new
		#if USE_MIDI_SYSTEM
			wsMidiButton(
		#else
			wsButton(
		#endif
			this,
			ID_LOOP_STOP_BUTTON,
			getLoopCommandName(LOOP_COMMAND_STOP_IMMEDIATE),
			20,
			320,
			120,
			380);

	pDubButton = new
		#if USE_MIDI_SYSTEM
			wsMidiButton(
		#else
			wsButton(
		#endif
			this,
			ID_LOOP_DUB_BUTTON,
			getLoopCommandName(LOOP_COMMAND_DUB_MODE),
			20,
			400,
			120,
			460);


	// register handler

	serial_midi_len = 0;
	m_pSerial = CCoreTask::Get()->GetKernel()->GetSerial();

	#if USE_SERIAL_INTERRUPTS
		if (m_pSerial)
		{
			LOG("Registering serial interrupt handler",0);
			m_pSerial->RegisterReceiveIRQHandler(this,staticSerialReceiveIRQHandler);
		}
	#endif

	LOG("uiWindow ctor finished",0);

}	// uiWindow ctor



// virtual
void uiWindow::updateFrame()
{
	#if !USE_SERIAL_INTERRUPTS
		// polling approach

		// This code allows for 1..5 to be typed into the buttons
		// or for packets 0x0b 0xb0 NN 0x7f where NN is
		// 21, 22, 23, 31, and 25, for buttons one through 5
		// to be handled.

		if (m_pSerial)
		{
			u8 buf[4];
			int num_read = m_pSerial->Read(buf,4);
			if (num_read == 1)
			{
				// the 1 case is old, nearly obsolete, to control directly from windows machine
				u16 button_num = buf[0] - '1';
				// the 1 case is old, nearly obsolete, to control directly from windows machine
				u16 button_num = c - '1';
				if (button_num == 7)
					pTheLooper->command(LOOP_COMMAND_DUB_MODE);
				if (button_num == 6)
					pTheLooper->command(LOOP_COMMAND_CLEAR_ALL);
				else if (button_num == 5)
					pTheLooper->command(LOOP_COMMAND_STOP_IMMEDIATE);
				else if (button_num == 4)
					pTheLooper->command(LOOP_COMMAND_CLEAR_ALL);
				else if (button_num < 4)
					pTheLooper->command(LOOP_COMMAND_TRACK_BASE + button_num);
			}

			// handle encapsulated midi messages

			else if (num_read == 4)
			{
				if (buf[0] == 0x0b &&			// CC messages
					buf[1] == 0xb0)
				{
					// CC's that map to volume controls
					// also works for the teensyExpression loop pedal

					if (buf[2] >= 0x65 &&
						buf[2] <= 0x69)
					{
						int control_num = buf[2] - 0x65;
						pTheLooper->setControl(control_num,buf[3]);
					}

					// CC's that map to buttons

					else if (buf[2] == LOOP_COMMAND_CC)
					{
						LOG("SERIAL_MIDI() loop command 0x%02x - %s",buf[3],getLoopCommandName(buf[3]));
						pTheLooper->command(buf[3]);
					}

					// unknown CCs

					else
					{
						LOG_WARNING("unknown serial midi Continuous Controller 0x%02",buf[2]);
						display_bytes("midi buffer",buf,4);
					}
				}

				// unknown midi serial messages

				else
				{
					LOG_WARNING("unknown serial midi command",0);
					display_bytes("unknown midi buffer",buf,4);
				}

				// weird number of bytes
			}
			else if (num_read)
			{
				LOG_WARNING("unexpected number of serial bytes: %d", num_read);
			}
		}
	#endif	// polling for serial midi


	#if 1
		logString_t *msg = pTheLooper->getNextLogString();
		if (msg)
		{
			CLogger::Get()->Write(msg->lname,LogNotice,*msg->string);
			delete msg->string;
			delete msg;
		}
	#endif

	wsWindow::updateFrame();
}




// virtual
u32 uiWindow::handleEvent(wsEvent *event)
{
	u32 result_handled = 0;
	u32 type = event->getEventType();
	u32 event_id = event->getEventID();
	u32 id = event->getID();
	// wsWindow *obj = event->getObject();
	LOG("handleEvent(%08x,%d,%d)",type,event_id,id);

	if (type == EVT_TYPE_BUTTON &&
		event_id == EVENT_CLICK)
	{
		if (id == ID_LOOP_STOP_BUTTON)
			pTheLooper->command(LOOP_COMMAND_STOP_IMMEDIATE);
		else if (id == ID_LOOP_DUB_BUTTON)
			pTheLooper->command(LOOP_COMMAND_DUB_MODE);
		else if (id >= ID_LOOP_TRACK_BUTTON_BASE &&
				 id < ID_LOOP_TRACK_BUTTON_BASE + NUM_TRACK_BUTTONS)
		{
			int track_num = id = ID_LOOP_TRACK_BUTTON_BASE;
			pTheLooper->command(LOOP_COMMAND_TRACK_BASE + track_num);
		}
		result_handled = 1;
	}

	if (!result_handled)
		result_handled = wsTopLevelWindow::handleEvent(event);

	return result_handled;
}



//-------------------------------------------
// interrupt driven serial midi
//-------------------------------------------

// static
void uiWindow::staticSerialReceiveIRQHandler(void *pThis, unsigned char c)
{
	((uiWindow *)pThis)->serialReceiveIRQHandler(c);

}

void uiWindow::serialReceiveIRQHandler(unsigned char c)
{
	if (!serial_midi_len)
	{
		if (c == 0x0b)
		{
			serial_midi_buf[serial_midi_len++] = c;		// start serial midi message
		}
		else
		{
			// the 1 case is old, nearly obsolete, to control directly from windows machine
			u16 button_num = c - '1';
			if (button_num == 7)
				pTheLooper->command(LOOP_COMMAND_DUB_MODE);
			if (button_num == 6)
				pTheLooper->command(LOOP_COMMAND_CLEAR_ALL);
			else if (button_num == 5)
				pTheLooper->command(LOOP_COMMAND_STOP_IMMEDIATE);
			else if (button_num == 4)
				pTheLooper->command(LOOP_COMMAND_CLEAR_ALL);
			else if (button_num < 4)
				pTheLooper->command(LOOP_COMMAND_TRACK_BASE + button_num);
		}
	}
	else
	{
		serial_midi_buf[serial_midi_len++] = c;		// add to buffer
		if (serial_midi_len == 4)
		{
			if (serial_midi_buf[0] == 0x0b &&			// CC messages
				serial_midi_buf[1] == 0xb0)
			{
				// CC's that map to volume controls
				// also works for the teensyExpression loop pedal

				if (serial_midi_buf[2] >= 0x65 &&
					serial_midi_buf[2] <= 0x69)
				{
					int control_num = serial_midi_buf[2] - 0x65;
					pTheLooper->setControl(control_num,serial_midi_buf[3]);
				}

				else if (serial_midi_buf[2] == LOOP_COMMAND_CC)
					pTheLooper->command(serial_midi_buf[3]);
			}

			serial_midi_len = 0;

		}	// reached 4 bytes of midi data
	}	// serial_midi_len != 0
}
