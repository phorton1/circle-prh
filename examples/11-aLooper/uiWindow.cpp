
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
	//
	// See note about using LOOPER_LOG() in loopMachine.cpp to
	// prevent "noise" when turning this on.


#define ID_LOOP_STATUS_BAR     		101
#define ID_VU1                		201
#define ID_VU2                		202
#define ID_VU3                		203
#define ID_VU4                		204
#define ID_VU_SLIDER           		250
#define ID_TRACK_CONTROL_BASE  		300
#define ID_LOOP_STOP_BUTTON         401
#define ID_LOOP_DUB_BUTTON    		402
#define ID_LOOP_TRACK_BUTTON_BASE   410


#define TOP_MARGIN 		32
#define BOTTOM_MARGIN  	60
#define RIGHT_MARGIN    	150

#define VU_TOP  	TOP_MARGIN+46
#define VU_BOTTOM   VU_TOP + 180

#define TRACK_VSPACE  	5
#define TRACK_HSPACE  	5

CSerialDevice *s_pSerial = 0;

// extern
void sendSerialMidiCC(int cc_num, int value)
{
	if (s_pSerial)
	{
		unsigned char midi_buf[4];
		midi_buf[0] = 0x0b;
		midi_buf[1] = 0xb0;
		midi_buf[2] = cc_num;
		midi_buf[3] = value;
		s_pSerial->Write((unsigned char *) midi_buf,4);
	}
}


//----------------------------------------------------------------

uiWindow::uiWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
	wsTopLevelWindow(pApp,id,xs,ys,xe,ye)
{
	LOG("uiWindow ctor",0);

	last_shown_dub_mode = 0;

	setBackColor(wsDARK_BLUE);

	int height = ye-ys+1;
    int width = xe-xs+1;
	int right_col = width - RIGHT_MARGIN;

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
		wsStaticText *pt1 = new wsStaticText(this,0,"THRU",right_col+6,TOP_MARGIN+23,right_col+42,TOP_MARGIN+39);
		pt1->setAlign(ALIGN_CENTER);
		pt1->setForeColor(wsWHITE);
		pt1->setFont(wsFont7x12);

		new vuSlider(this,ID_VU2,right_col+8, VU_TOP, right_col+40, VU_BOTTOM, false, 12,
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
		wsStaticText *pt2 = new wsStaticText(this,0,"LOOP",right_col+50,TOP_MARGIN+23,right_col+86,TOP_MARGIN+39);
		pt2->setAlign(ALIGN_CENTER);
		pt2->setForeColor(wsWHITE);
		pt2->setFont(wsFont7x12);

		new vuSlider(this,ID_VU3, right_col+52, VU_TOP, right_col+84, VU_BOTTOM, false, 12,
			METER_LOOP,
			CONTROL_LOOP_VOLUME,
			-1,		// cable
			-1,		// 0 based channel
			MIDI_EVENT_TYPE_CC,			// jumpy CC for foot pedal
			0x0F);						// bottom right knob
	#endif

	// RIGHT - MIX vu

	#if WITH_METERS
		wsStaticText *pt3 = new wsStaticText(this,0,"MIX",right_col+94,TOP_MARGIN+23,right_col+130,TOP_MARGIN+39);
		pt3->setAlign(ALIGN_CENTER);
		pt3->setForeColor(wsWHITE);
		pt3->setFont(wsFont7x12);

		new vuSlider(this,ID_VU4, right_col+96, VU_TOP, right_col+128, VU_BOTTOM, false, 12,
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
	int cwidth = (width-RIGHT_MARGIN-1-TRACK_HSPACE*(LOOPER_NUM_TRACKS+1)) / LOOPER_NUM_TRACKS;
	int step = TRACK_HSPACE;
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
				step+10,
				btop+5,
				step + cwidth - 10,
				btop+49,
				BTN_STYLE_USE_ALTERNATE_COLORS);
		pTrackButtons[i]->setFont(wsFont12x16);
		pTrackButtons[i]->setAltBackColor(wsSLATE_GRAY);

		step += cwidth + TRACK_HSPACE;
		LOG("finished creating ui_track(%d)",i);
	}

	stop_button_cmd = 0;
		// goes with the blank below
	pStopButton = new
		#if USE_MIDI_SYSTEM
			wsMidiButton(
		#else
			wsButton(
		#endif
			this,
			ID_LOOP_STOP_BUTTON,
			"",		// button does not start off with a function: getLoopCommandName(LOOP_COMMAND_STOP),
			right_col + 3,
			btop-65+5,
			width - 12,
			btop-65+49,
			BTN_STYLE_USE_ALTERNATE_COLORS,
			WIN_STYLE_CLICK_LONG);
	pStopButton->setAltBackColor(wsSLATE_GRAY);
	pStopButton->setFont(wsFont12x16);


	pDubButton = new
		#if USE_MIDI_SYSTEM
			wsMidiButton(
		#else
			wsButton(
		#endif
			this,
			ID_LOOP_DUB_BUTTON,
			getLoopCommandName(LOOP_COMMAND_DUB_MODE),
			right_col + 3,
			btop+5,
			width - 12,
			btop+49,
			BTN_STYLE_USE_ALTERNATE_COLORS,
			WIN_STYLE_CLICK_LONG);
	pDubButton->setAltBackColor(wsSLATE_GRAY);
	pDubButton->setFont(wsFont12x16);


	// register handler

	serial_midi_len = 0;
	s_pSerial = CCoreTask::Get()->GetKernel()->GetSerial();

	#if USE_SERIAL_INTERRUPTS
		if (s_pSerial)
		{
			LOG("Registering serial interrupt handler",0);
			s_pSerial->RegisterReceiveIRQHandler(this,staticSerialReceiveIRQHandler);
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

		if (s_pSerial)
		{
			u8 buf[4];
			int num_read = s_pSerial->Read(buf,4);
			if (num_read == 1)
			{
				// the 1 case is old, nearly obsolete, to control directly from windows machine
				u16 button_num = buf[0] - '1';
				// the 1 case is old, nearly obsolete, to control directly from windows machine
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
					handleSerialCC(buf[2],buf[3]);
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

	// STOP button is blank if not running,
	// says STOP if running and pending command is not STOP
	// says STOP! if running AND pending command is STOP

	bool running = pTheLooper->getRunning();
	u16  pending = pTheLooper->getPendingCommand();
	u16 t_command = running ?
		pending == LOOP_COMMAND_STOP ?
			LOOP_COMMAND_STOP_IMMEDIATE :
			LOOP_COMMAND_STOP : 0;

	if (stop_button_cmd != t_command)
	{
		stop_button_cmd = t_command;
		pStopButton->setText(getLoopCommandName(t_command));
		sendSerialMidiCC(LOOP_STOP_CMD_STATE_CC,stop_button_cmd);
	}

	// DUB Button changes based on looper dub mode,
	// and notifies the TE of any changes

	bool dub_mode = pTheLooper->getDubMode();
	if (dub_mode != last_shown_dub_mode)
	{
		LOG("updateState dub mode=%d",dub_mode);
		last_shown_dub_mode = dub_mode;
		int bc = dub_mode ? wsORANGE : defaultButtonReleasedColor;
		pDubButton->setBackColor(bc);
		pDubButton->setStateBits(WIN_STATE_DRAW);
		sendSerialMidiCC(LOOP_DUB_STATE_CC,dub_mode);
	}

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
	LOG("uiWindow::handleEvent(%08x,%d,%d)",type,event_id,id);

	if (type == EVT_TYPE_WINDOW &&
	   event_id == EVENT_LONG_CLICK)
	{
		if (id == ID_LOOP_STOP_BUTTON || id == ID_LOOP_DUB_BUTTON)
			pTheLooper->command(LOOP_COMMAND_CLEAR_ALL);
	}
	else if (type == EVT_TYPE_BUTTON &&
		     event_id == EVENT_CLICK)
	{
		if (id == ID_LOOP_STOP_BUTTON)
		{
			if (stop_button_cmd)
				pTheLooper->command(stop_button_cmd);
		}
		else if (id == ID_LOOP_DUB_BUTTON)
		{
			pTheLooper->command(LOOP_COMMAND_DUB_MODE);
		}
		else if (id >= ID_LOOP_TRACK_BUTTON_BASE &&
				 id < ID_LOOP_TRACK_BUTTON_BASE + NUM_TRACK_BUTTONS)
		{
			int track_num = id - ID_LOOP_TRACK_BUTTON_BASE;
			pTheLooper->command(LOOP_COMMAND_TRACK_BASE + track_num);
		}
		result_handled = 1;
	}
	else if (type == EVT_TYPE_WINDOW &&
		     event_id == EVENT_CLICK &&
			 id >= ID_CLIP_BUTTON_BASE &&
			 id <= ID_CLIP_BUTTON_BASE + LOOPER_NUM_TRACKS*LOOPER_NUM_LAYERS)
	{
		int num = id - ID_CLIP_BUTTON_BASE;
		int track_num = num / LOOPER_NUM_LAYERS;
		int clip_num = num % LOOPER_NUM_LAYERS;

		publicTrack *pTrack = pTheLooper->getPublicTrack(track_num);
		publicClip  *pClip  = pTrack->getPublicClip(clip_num);
		bool mute = pClip->isMuted();

		LOG("setting clip(%d,%d) mute=%d",track_num,clip_num,!mute);
		pClip->setMute(!mute);
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
				handleSerialCC(serial_midi_buf[2],serial_midi_buf[3]);
			}
			serial_midi_len = 0;

		}	// reached 4 bytes of midi data
	}	// serial_midi_len != 0
}



void uiWindow::handleSerialCC(u8 cc_num, u8 value)
{
	// CC's that map to volume controls
	// also works for the teensyExpression loop pedal

	if (cc_num >= LOOP_CONTROL_BASE_CC &&
		cc_num < LOOP_CONTROL_BASE_CC + LOOPER_NUM_CONTROLS)
	{
		int control_num = cc_num - LOOP_CONTROL_BASE_CC;
		pTheLooper->setControl(control_num,value);
	}

	// CC's that map to buttons

	else if (cc_num == LOOP_COMMAND_CC)
	{
		pTheLooper->command(value);
	}

	else if (cc_num >= CLIP_VOL_BASE_CC &&
			 cc_num <= CLIP_VOL_BASE_CC  + LOOPER_NUM_TRACKS * LOOPER_NUM_LAYERS)
	{
		int num = cc_num - CLIP_VOL_BASE_CC;
		int track_num = num / LOOPER_NUM_LAYERS;
		int clip_num = num % LOOPER_NUM_LAYERS;
		publicTrack *pTrack = pTheLooper->getPublicTrack(track_num);
		publicClip *pClip = pTrack->getPublicClip(clip_num);
		pClip->setVolume(value);
	}
	else if (cc_num >= CLIP_MUTE_BASE_CC &&
			 cc_num <= CLIP_MUTE_BASE_CC  + LOOPER_NUM_TRACKS * LOOPER_NUM_LAYERS)
	{
		int num = cc_num - CLIP_MUTE_BASE_CC;
		int track_num = num / LOOPER_NUM_LAYERS;
		int clip_num = num % LOOPER_NUM_LAYERS;
		publicTrack *pTrack = pTheLooper->getPublicTrack(track_num);
		publicClip *pClip = pTrack->getPublicClip(clip_num);
		pClip->setMute(value);
	}
}
