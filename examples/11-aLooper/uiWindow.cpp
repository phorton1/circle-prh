
#include "uiWindow.h"
#include <circle/logger.h>
#include <utils/myUtils.h>

#include "looper.h"
#include "uiStatusBar.h"
#include "uiTrack.h"


#define log_name  "loopwin"


#define TOP_MARGIN 		32
#define BOTTOM_MARGIN  	50
#define LEFT_MARGIN    	150

#define BUTTON_SPACING 	20
#define TRACK_VSPACE  	5
#define TRACK_HSPACE  	5


		
#define ID_RECORD_BUTTON    201
#define ID_PLAY_BUTTON      202
#define ID_NEXT_BUTTON      203
#define ID_STOP_BUTTON      204

#define ID_LOOP_STATUS_BAR  101

#define ID_VU1      		301
#define ID_VU2      		302

#define	ID_TRACK_CONTROL_BASE 400 	// ..411

#define ID_LOOP_BUTTON1    201		// RECORD - RECORD_NEXT
#define ID_LOOP_BUTTON2    202		// PLAY_IMMEDIATE/PLAY_NEXT - PLAY_IMMEDIATE - STOP_IMMEDIATE
#define ID_LOOP_BUTTON3    203		// NEXT
#define ID_LOOP_BUTTON4    204		// STOP(ZERO) - CLEAR_ALL


uiWindow::uiWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
	wsTopLevelWindow(pApp,id,xs,ys,xe,ye)
{
	setBackColor(wsDARK_BLUE);
	
	int height = ye-ys+1;
    int width = xe-xs+1;
    int bwidth = (width-LEFT_MARGIN-BUTTON_SPACING*5) / 4;
    int offset = LEFT_MARGIN + BUTTON_SPACING;
    int btop = height-BOTTOM_MARGIN;
    
    new wsMidiButton(this,ID_RECORD_BUTTON,"REC",offset,btop+10,offset+bwidth,btop+39);
    offset += bwidth + BUTTON_SPACING;
    new wsMidiButton(this,ID_PLAY_BUTTON,"PLAY",offset,btop+10,offset+bwidth,btop+39);
    offset += bwidth + BUTTON_SPACING;
    new wsMidiButton(this,ID_NEXT_BUTTON,"NEXT",offset,btop+10,offset+bwidth,btop+39);
    offset += bwidth + BUTTON_SPACING;
    new wsMidiButton(this,ID_STOP_BUTTON,"STOP",offset,btop+10,offset+bwidth,btop+39);
	
	new uiStatusBar(this,ID_LOOP_STATUS_BAR,0,0,width-165,TOP_MARGIN-1);
	
	#if 1
		awsVuMeter *pInputVU1 = new awsVuMeter(this,ID_VU1,width-150-1,2,width-6,14,1,12);
		awsVuMeter *pInputVU2 = new awsVuMeter(this,ID_VU2,width-150-1,16,width-6,28,1,12);
		pInputVU1->setAudioDevice("tdmi",0,0);
		pInputVU2->setAudioDevice("tdmi",0,1);
		
		awsVuMeter *pOutputVU1 = new awsVuMeter(this,ID_VU1,40,TOP_MARGIN+40,58,TOP_MARGIN+40+240+1,0,12);
		awsVuMeter *pOutputVU2 = new awsVuMeter(this,ID_VU1,60,TOP_MARGIN+40,78,TOP_MARGIN+40+240+1,0,12);
		pOutputVU1->setAudioDevice("looper",0,0);
		pOutputVU2->setAudioDevice("looper",0,1);
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
	
}	// LooperWidnow::uiWindow()


		
// virtual
u32 uiWindow::handleEvent(wsEvent *event)
{
	u32 result_handled = 0;
	u32 type = event->getEventType();
	u32 event_id = event->getEventID();
	u32 id = event->getID();
	// wsWindow *obj = event->getObject();
	// LOG("handleEvent(%08x,%d,%d)",type,event_id,id);
	// 
	
	if (type == EVT_TYPE_BUTTON &&
		event_id == EVENT_CLICK)
	{
		if (id == ID_RECORD_BUTTON)
		{
			LOG("RECORD BUTTON PRESSED",0);
			pLooper->command(LOOP_COMMAND_RECORD);
		}
		else if (id == ID_PLAY_BUTTON)
		{
			LOG("PLAY BUTTON PRESSED",0);
			pLooper->command(LOOP_COMMAND_PLAY);
		}
		else if (id == ID_NEXT_BUTTON)
		{
			LOG("NEXT BUTTON PRESSED",0);
			pLooper->command(LOOP_COMMAND_STOP);
			pLooper->command(LOOP_COMMAND_CLEAR_ALL);
		}
		else if (id == ID_STOP_BUTTON)
		{
			LOG("STOP BUTTON PRESSED",0);
			pLooper->command(LOOP_COMMAND_STOP);
		}
		
	}

	if (!result_handled)
		result_handled = wsTopLevelWindow::handleEvent(event);
	
	return result_handled;
}





