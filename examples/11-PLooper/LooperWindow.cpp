
#include "LooperWindow.h"
#include <circle/logger.h>
#include <circle/timer.h>
#include <utils/myUtils.h>
#include "Looper.h"

#define log_name  "loopwin"


#define TOP_MARGIN 30
#define BOTTOM_MARGIN  50
#define BUTTON_SPACING 20

#define ID_LOOPER_STATUS_WINDOW  101

#define ID_RECORD_BUTTON    201
#define ID_PLAY_BUTTON      202
#define ID_NEXT_BUTTON      203
#define ID_STOP_BUTTON      204


class LooperStatusBar : public wsWindow
{
	public:	

		LooperStatusBar(wsWindow *pParent,s32 xs, s32 ys, s32 xe, s32 ye) :
			wsWindow(pParent,ID_LOOPER_STATUS_WINDOW,xs,ys,xe,ye)
		{
			setBackColor(wsDARK_GREEN);
			
			m_update_time = 0;
			m_blocks_used = 0;			
			m_blocks_free = 0;			
			m_selected_track = 0;		
			m_num_tracks = 0;		
			m_selected_clip = 0;
			m_num_clips = 0;
		}
		
		
		virtual void update()
		{
			unsigned cur_time = CTimer::Get()->GetTicks();		// 100's of a second
			
			if (cur_time < m_update_time || cur_time > m_update_time + 10)   // update 10 times a second
			{
				m_update_time = cur_time;
				LoopBuffer *pLoopBuffer = pLooper ? pLooper->getLoopBuffer() : 0;
				if (pLooper && pLoopBuffer)
				{
					u32 blocks_used 	= pLoopBuffer->getUsedBlocks();
					u32 blocks_free 	= pLoopBuffer->getFreeBlocks();
					u16 num_tracks 		= pLooper->getNumUsedTracks();
					LoopTrack *pTrack   = pLooper->getCurTrack();
					u16 num_clips		= pTrack->getNumClips();
					u32 base_pos        = pTrack->getClip(0)->getCurBlock();
					u32 base_length     = pTrack->getClip(0)->getNumBlocks();
					
					if (blocks_used		!= m_blocks_used	||
						blocks_free 	!= m_blocks_free 	||
						num_tracks 		!= m_num_tracks 	||
						num_clips       != m_num_clips      ||
						base_pos	    != m_base_pos  		||
						base_length     != m_base_length    )
					{
						m_blocks_used    = blocks_used;
						m_blocks_free 	 = blocks_free;
						m_num_tracks 	 = num_tracks;
						m_num_clips      = num_clips;
						m_base_pos		 = base_pos;
						m_base_length    = base_length;
						setBit(m_state,WIN_STATE_DRAW);
					}
				}
			}
			
			// paranoically call base class update on every up-call
			// in case window moves (which it wont) or it's the first
			// time (which could be solved by setting m_update_time
			// to a high value to begin)
			
			wsWindow::update();
		}
		
		
		
		void onDraw()
		{
			// printf("LooperStatusBar::onDraw()\n",0);
			
			wsWindow::onDraw();
			
			CString msg;
			msg.Format("%8d/%-8d  tracks:%d  clips:%d    base %8d/%-8d",
				m_blocks_used,
				m_blocks_free,   
				m_num_tracks,	   
				m_num_clips,
				m_base_pos,
				m_base_length);

			m_pDC->setFont(m_pFont);
			m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
			m_pDC->putText(
				m_back_color,
				m_fore_color,
				m_rect_client,
				m_align,
				1,2,
				(const char *)msg);
		}

	private:
		
		unsigned m_update_time;
		
		u32 m_blocks_used;
		u32 m_blocks_free;
		u16 m_selected_track;
		u16 m_num_tracks;
		u16 m_selected_clip;
		u16 m_num_clips;
		u32 m_base_pos;
		u32 m_base_length;
	
};




LooperWindow::LooperWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
	wsTopLevelWindow(pApp,id,xs,ys,xe,ye)
{
	setBackColor(wsDARK_BLUE);
	
	int height = ye-ys+1;
    int width = xe-xs+1;
    int bwidth = (width-BUTTON_SPACING*5) / 4;
    int offset = BUTTON_SPACING;
    int btop = height-BOTTOM_MARGIN;
    
    m_pRecordButton = new wsMidiButton(this,ID_RECORD_BUTTON,"REC",offset,btop+10,offset+bwidth,btop+39);
    offset += bwidth + BUTTON_SPACING;
    m_pPlayButton = new wsMidiButton(this,ID_PLAY_BUTTON,"PLAY",offset,btop+10,offset+bwidth,btop+39);
    offset += bwidth + BUTTON_SPACING;
    m_pNextButton = new wsMidiButton(this,ID_NEXT_BUTTON,"NEXT",offset,btop+10,offset+bwidth,btop+39);
    offset += bwidth + BUTTON_SPACING;
    m_pStopButton = new wsMidiButton(this,ID_STOP_BUTTON,"STOP",offset,btop+10,offset+bwidth,btop+39);
	
	new LooperStatusBar(this,0,0,width-1,TOP_MARGIN-1);
	
}

		
// virtual
u32 LooperWindow::handleEvent(wsEvent *event)
{
	u32 result_handled = 0;
	u32 type = event->getEventType();
	u32 event_id = event->getEventID();
	u32 id = event->getID();
	// wsWindow *obj = event->getObject();
	// LOG("handleEvent(%08x,%d,%d)",type,event_id,id);
	// 
	
	if (type == EVT_TYPE_BUTTON &&
		event_id == BTN_EVENT_PRESSED)
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





