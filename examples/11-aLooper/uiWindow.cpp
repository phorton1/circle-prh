
#include "uiWindow.h"
#include <circle/logger.h>
#include <utils/myUtils.h>

#include "Looper.h"
#include "uiStatusBar.h"
#include "uiTrack.h"
#include "vuSlider.h"


#define log_name  "loopwin"


#define TOP_MARGIN 		32
#define BOTTOM_MARGIN  	50
#define LEFT_MARGIN    	150

#define VU_TOP  	TOP_MARGIN+40
#define VU_BOTTOM   VU_TOP + 180


#define BUTTON_SPACING 	20
#define TRACK_VSPACE  	5
#define TRACK_HSPACE  	5

#define ID_LOOP_STATUS_BAR     101
#define ID_VUI1                201	// codec input
#define ID_VUI2                202
#define ID_VUO1                203	// final output (looper or output mixer)
#define ID_VUO2                204
#define ID_VUAI1               205	// input amp if present
#define ID_VUAI2               206
#define ID_VUOM1               207	// looper output
#define ID_VUOM2               208
#define ID_VUT1 	           207	// thru output (if NO_LOOPER_THRU)
#define ID_VUT2                208  // copy of input amp or input

#define ID_VU_SLIDER           250

#define ID_TRACK_CONTROL_BASE  300  // ..311
#define ID_LOOP_BUTTON_BASE    400  // ..403  

//----------------------------------------------------------------

u16 uiWindow::getButtonFunction(u16 num)
{
	u16 state = pLooper->getLoopState();

	if (num == 0)
	{
		u16 cur_track_num = pLooper->getCurTrackNum();
		u16 sel_track_num = pLooper->getSelectedTrackNum();
		loopClip *pClip = pLooper->getSelectedTrack()->getSelectedClip();
		
		if (!pClip->getNumBlocks())
		{
			if (pClip->getClipState() == LOOP_CLIP_STATE_RECORDING)
				return LOOP_COMMAND_PLAY;
			else
				return LOOP_COMMAND_RECORD;
		}
		
		if ((state == LOOP_STATE_PLAYING ||
			 state == LOOP_STATE_RECORDING) &&
			 cur_track_num == sel_track_num)
			return LOOP_COMMAND_STOP;
		
		return LOOP_COMMAND_PLAY;
	}
	if (num == 1)
	{
		loopTrack *pTrack = pLooper->getSelectedTrack();
		if (pTrack->getNumClips())
			return LOOP_COMMAND_SELECT_NEXT_CLIP;
		
		// if (state == LOOP_STATE_RECORDING ||
		// 	pLooper->getNumUsedTracks())
		// 	return LOOP_COMMAND_PLAY;
	}
	if (num == 2)
	{
		if (pLooper->getNumUsedTracks())
			return LOOP_COMMAND_SELECT_NEXT_TRACK;
	}
	if (num == 3)
	{
		if (state == LOOP_STATE_RECORDING ||
			state == LOOP_STATE_PLAYING)
			return LOOP_COMMAND_STOP_IMMEDIATE;
		if (num != 0)
			return LOOP_COMMAND_CLEAR_ALL;
	}
	return LOOP_COMMAND_NONE;
}



//----------------------------------------------------------------



uiWindow::uiWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
	wsTopLevelWindow(pApp,id,xs,ys,xe,ye)
{
	setBackColor(wsDARK_BLUE);
	
	int height = ye-ys+1;
    int width = xe-xs+1;
    int bwidth = (width-LEFT_MARGIN-BUTTON_SPACING*(NUM_LOOP_BUTTONS+1)) / NUM_LOOP_BUTTONS;
    int offset = LEFT_MARGIN + BUTTON_SPACING;
    int btop = height-BOTTOM_MARGIN;

	new uiStatusBar(this,ID_LOOP_STATUS_BAR,165,0,width-165,TOP_MARGIN-1);
	
	#if 1
		// We use find() to located the codec (input device)
		// with any name (0) and any instance (-1)

		// TOP LEFT INPUT VU METER - CODEC audio input, period
		
		AudioStream *pInputDevice = AudioSystem::find(AUDIO_DEVICE_INPUT,0,-1);
		awsVuMeter *pInputVU1 = new awsVuMeter(this,ID_VUI1,6, 2,151,14,1,12);
		awsVuMeter *pInputVU2 = new awsVuMeter(this,ID_VUI2,6,16,151,28,1,12);
		pInputVU1->setAudioDevice(pInputDevice->getName(),0,0);
		pInputVU2->setAudioDevice(pInputDevice->getName(),0,1);

		// TOP RIGHT OUTPUT VU METER - final mixer or looper output
		
		awsVuMeter *pOutputVU1 = new awsVuMeter(this,ID_VUO1,width-150-1, 2,width-6,14,1,12);
		awsVuMeter *pOutputVU2 = new awsVuMeter(this,ID_VUO2,width-150-1,16,width-6,28,1,12);
		#if USE_OUTPUT_MIXER
			pOutputVU1->setAudioDevice("mixer",0,0);
			pOutputVU2->setAudioDevice("mixer",1,0);
		#else
			pOutputVU1->setAudioDevice("looper",0,0);
			pOutputVU2->setAudioDevice("looper",0,1);
		#endif
		
		// LEFT BAR - INPUT vu if using INPUT_AMP
		
		#if USE_INPUT_AMP
			wsStaticText *pt1 = new wsStaticText(this,0,"IN",6,TOP_MARGIN+23,44,TOP_MARGIN+39);
			pt1->setAlign(ALIGN_CENTER);
			pt1->setForeColor(wsWHITE);
			pt1->setFont(wsFont7x12);
			
			awsVuMeter *pAmpVU1 = new awsVuMeter(this,ID_VUAI1,  8, VU_TOP, 23, VU_BOTTOM, 0, 12);
			awsVuMeter *pAmpVU2 = new awsVuMeter(this,ID_VUAI2, 25, VU_TOP, 40, VU_BOTTOM, 0, 12);
			pAmpVU1->setAudioDevice("amp",0,0);
			pAmpVU2->setAudioDevice("amp",1,0);
		#endif

		// A general issue is that you have to connect a VU meter to an output
		//
		// So you can't see what's going on INSIDE a mixer or final output device.
		// Likewise, you cannot see "before" the codec input gain stage,
		// you can only show the signal AFTER it has gone through the input stage,
		// or BEFORE it has gone through the final output stage.

		#if USE_OUTPUT_MIXER
			// main output vu
			pOutputVU1->setAudioDevice("mixer",0,0);
			pOutputVU2->setAudioDevice("mixer",1,0);

			// loop output vu
			
		#if 1
			wsStaticText *pt2 = new wsStaticText(this,0,"LOOP",46,TOP_MARGIN+23,86,TOP_MARGIN+39);
			// LOG("pt2=%08x",(u32)pt2);
			pt2->setAlign(ALIGN_CENTER);
			pt2->setForeColor(wsWHITE);
			pt2->setFont(wsFont7x12);
		#endif
		#if 1
			#if 0
				wsStaticText *pWin = new wsStaticText(this,ID_VU_SLIDER,"blah", 5, VU_BOTTOM+10, 100, VU_BOTTOM+30);
				pWin->setBackColor(wsCYAN);
				LOG("pWin=%08x",(u32)pWin);
			#else
				/// vuSlider *pSlider =
				new vuSlider(this,ID_VU_SLIDER,50, VU_TOP, 140, VU_BOTTOM,
					0,		// channel 1, 0 based, as programmed on MPD21
					0x0D);	// 0x0D = the middle right knob on MPD218
			#endif
		#endif
			
			awsVuMeter *pLoopVU1 = new awsVuMeter(this,ID_VUOM1, 50, VU_TOP, 65, VU_BOTTOM, 0, 12);
			awsVuMeter *pLoopVU2 = new awsVuMeter(this,ID_VUOM2, 67, VU_TOP, 82, VU_BOTTOM, 0, 12);
			pLoopVU1->setAudioDevice("looper",0,0);
			pLoopVU2->setAudioDevice("looper",0,1);
			
			
		#endif
		
	#endif
	
	int cheight = height-TOP_MARGIN-BOTTOM_MARGIN-TRACK_VSPACE*2;
	int cwidth = (width-LEFT_MARGIN-1-TRACK_HSPACE*(LOOPER_NUM_TRACKS+1)) / LOOPER_NUM_TRACKS;
	int step = LEFT_MARGIN + TRACK_HSPACE;
	
	for (int i=0; i<LOOPER_NUM_TRACKS; i++)
	{
		new uiTrack(
			i,
			this,
			ID_TRACK_CONTROL_BASE + i,
			step,
			TOP_MARGIN + TRACK_VSPACE,
			step + cwidth -1,
			TOP_MARGIN + TRACK_VSPACE + cheight - 1);

		step += cwidth + TRACK_HSPACE;
	}

	last_loop_state = pLooper->getLoopState();				// NONE = 0
	last_num_used_tracks = pLooper->getNumUsedTracks();		// 0
	for (int i=0; i<NUM_LOOP_BUTTONS; i++)
	{
		u16 fxn = last_button_fxn[i] = getButtonFunction(i);
		pLoopButton[i] = new wsMidiButton(
			this,
			ID_LOOP_BUTTON_BASE + i,
			pLooper->getCommandName(fxn),
			offset,
			btop+10,
			offset+bwidth,
			btop+39);
		offset += bwidth + BUTTON_SPACING;
	}
	
}	// uiWindow ctor



// virtual
void uiWindow::updateFrame()
{
	for (int i=0; i<NUM_LOOP_BUTTONS; i++)
	{
		u16 fxn = getButtonFunction(i);
		if (fxn != last_button_fxn[i])
		{
			pLoopButton[i]->setText(pLooper->getCommandName(fxn));
			last_button_fxn[i] = fxn;
		}
	}
	
	#if 0
		u16 state = pLooper->getLoopState();
		u16 num = pLooper->getNumUsedTracks();
		if (state != last_loop_state ||
			num   != last_num_used_tracks)
		{
			last_loop_state = state;
			last_num_used_tracks = num;
			setBit(m_state,WIN_STATE_DRAW);
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
			const char *command_name = pLooper->getCommandName(loop_command);
			
			LOG("BUTTON(%d) command=%d '%s') PRESSED",button_num,loop_command,command_name);
			
			pLooper->command(loop_command);
			result_handled = 1;
		}
	}

	if (!result_handled)
		result_handled = wsTopLevelWindow::handleEvent(event);
	
	return result_handled;
}





