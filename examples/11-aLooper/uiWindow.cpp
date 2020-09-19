
#include "uiWindow.h"
#include <circle/logger.h>
#include <utils/myUtils.h>

#include "Looper.h"
#include "uiStatusBar.h"
#include "uiTrack.h"
#include "vuSlider.h"

#include <system/std_kernel.h>


#define log_name  "loopwin"


#define TOP_MARGIN 		32
#define BOTTOM_MARGIN  	50
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

#define ID_TRACK_CONTROL_BASE  300  // ..311
#define ID_LOOP_BUTTON_BASE    400  // ..403


//----------------------------------------------------------------

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



//----------------------------------------------------------------

uiWindow::uiWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
	wsTopLevelWindow(pApp,id,xs,ys,xe,ye)
{
	LOG("uiWindow ctor",0);

	last_running = 0;
	last_num_used_tracks = 0;

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

	#if 1

		LOG("creating ui_tracks",0);

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

			step += cwidth + TRACK_HSPACE;
			LOG("finished creating ui_track(%d)",i);
		}
	#endif


	// LOOPER BUTTONS
	// This MPD preset outputs a note on and off event for the
	// notes numbered 0x20 .. 0x2f (32 thru 47) from the top
	// left corner.

	#if 1

		LOG("creating buttons",0);

		// buttons hardwired for 600x800 screen or whatever

		#define BUTTON_LEFT     40
		#define BUTTON_WIDTH    130
		#define BUTTON_SPACING 	20

	    int offset = BUTTON_LEFT;
		int btop = height-BOTTOM_MARGIN;
		for (int i=0; i<NUM_LOOP_BUTTONS; i++)
		{
			LOG("creating ui_button(%d)",i);
			u16 fxn = last_button_fxn[i] = getButtonFunction(i);
			pLoopButton[i] = new

			#if USE_MIDI_SYSTEM
				wsMidiButton(
			#else
				wsButton(
			#endif
				this,
				ID_LOOP_BUTTON_BASE + i,
				getLoopCommandName(fxn),
				offset,
				btop+10,
				offset+BUTTON_WIDTH,
				btop+39

			#if USE_MIDI_SYSTEM
				,
				-1,
				-1,
				0x2c+i);
			#else
				);
			#endif

			offset += BUTTON_WIDTH + BUTTON_SPACING;
			LOG("finished creating ui_button(%d)",i);
		}
	#endif

	LOG("uiWindow ctor finished",0);

}	// uiWindow ctor



// virtual
void uiWindow::updateFrame()
{
	#if 1
			// use serial port input keys 1..5 for buttons 0..4
		CSerialDevice *pSerial = CCoreTask::Get()->GetKernel()->GetSerial();

		// This code allows for 1..5 to be typed into the buttons
		// or for packets 0x0b 0xb0 NN 0x7f where NN is
		// 21, 22, 23, 31, and 25, for buttons one through 5
		// to be handled.

		if (pSerial)
		{
			u8 buf[4];
			int num_read = pSerial->Read(buf,4);
			if (num_read == 1)
			{
				// the 1 case is old, nearly obsolete, to control directly from windows machine
				u16 button_num = buf[0] - '1';
				u16 loop_command = getButtonFunction(button_num);
				LOG("SERIAL(%c) button=%d command=%d '%s' RECEIVED",buf[0],button_num,loop_command,getLoopCommandName(loop_command));
				pTheLooper->command(loop_command);
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

					else if (
						buf[2] == 21 ||
					    buf[2] == 22 ||
					    buf[2] == 23 ||
					    buf[2] == 31 ||
					    buf[2] == 25)
					{
						int button_num =
							buf[2] == 21 ? 0 :
							buf[2] == 22 ? 1 :
							buf[2] == 23 ? 2 :
							buf[2] == 31 ? 3 :
							4;

						u16 loop_command = getButtonFunction(button_num);
						LOG("SERIAL_MIDI() button=%d command=%d '%s' RECEIVED",button_num,loop_command,getLoopCommandName(loop_command));
						pTheLooper->command(loop_command);
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
	#endif

	for (int i=0; i<NUM_LOOP_BUTTONS; i++)
	{
		u16 fxn = getButtonFunction(i);
		if (fxn != last_button_fxn[i])
		{
			pLoopButton[i]->setText(getLoopCommandName(fxn));
			last_button_fxn[i] = fxn;
		}
	}

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
		if (id >= ID_LOOP_BUTTON_BASE &&
			id < ID_LOOP_BUTTON_BASE + NUM_LOOP_BUTTONS)
		{
			u16 button_num = id - ID_LOOP_BUTTON_BASE;
			u16 loop_command = getButtonFunction(button_num);

			LOG("BUTTON(%d) command=%d '%s' PRESSED",button_num,loop_command,getLoopCommandName(loop_command));

			pTheLooper->command(loop_command);
			result_handled = 1;
		}
	}

	if (!result_handled)
		result_handled = wsTopLevelWindow::handleEvent(event);

	return result_handled;
}
