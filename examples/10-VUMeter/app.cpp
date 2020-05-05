// 10-VUMeter
//
// My first attempt to combine UI and audio systems.
//
// This file initializes the UI system

#include <ws/ws.h>
#include <circle/logger.h>
#include <system/std_kernel.h>
#include <utils/myUtils.h>
#include "awsVuMeter.h"
#include "awsLoopTrackButton.h"

#define TEST_SOFT_SERIAL 1
#if TEST_SOFT_SERIAL
	#include <system/softSerial.h>
	softSerial *gp_softSerial = 0;
#endif


#define log_name  "app"

	
//------------------------------------------------------------
// define the layout, ids, windows, and objects of the UI
//------------------------------------------------------------

#define TOP_MARGIN  50
#define BOTTOM_MARGIN 50


#define ID_WIN_FRAME    1
#define ID_WIN_TOP      10
#define ID_WIN_CONTENT  20
#define ID_WIN_BOTTOM   30

#define ID_VU1			101
#define ID_VU2			102
#define ID_BUTTON1    	101

#define ID_TEXT1      	201

#define ID_MENU1         		500
#define ID_MENU1_OPTION1		501
#define ID_MENU1_OPTION2        502
#define ID_MENU1_OPTION3		503

#define ID_LOOP_BUTTON_BASE     600		// ..615

#define ID_DLG  1000
#define ID_BUTTON_CLOSE  1001


#define vuMeter(id)  ((awsVuMeter *)findChildByID(id))
#define button(id)   ((wsButton *)findChildByID(id))
#define stext(id)    ((wsStaticText *)findChildByID(id))
#define box(id)      ((wsCheckbox *)findChildByID(id))
#define menu(id)     ((wsMenu *)findChildByID(id))


awsVuMeter *s_pVU1 = 0;
awsVuMeter *s_pVU2 = 0;


class dialogWindow : public wsTopLevelWindow
	// a popup test dialog window
{
	public:
		
		dialogWindow(wsTopLevelWindow *pPrevTop, u16 id, s32 x, s32 y, s32 width, s32 height) :
			wsTopLevelWindow(pPrevTop->getApplication(),id,x,y,x + width-1,y + height-1,
				WIN_STYLE_3D | WIN_STYLE_POPUP),
			m_pPrevTop(pPrevTop)
		{
			setBackColor(wsDARK_BLUE);
			setForeColor(wsWHITE);
			
			const char *msg =
				"This is a test dialog window\n\n"
				"It presents a \"close\" button\n"
				"to allow you to close it.\n\n"
				"Clicking anywhere outside of it\n"
				"should close it too. It does not clip text very well.";
				
			wsStaticText *text = new wsStaticText(this,
				0, msg,
				10, 10, getWidth()-10-1, getHeight()-60-1);
			text->setAlign(ALIGN_TOP_LEFT);
			text->setBackColor(wsPURPLE);
			
			new wsButton(this,
				ID_BUTTON_CLOSE,
				"close",
				getWidth()-95,
				getHeight()-50,
				getWidth()-15-1,
				getHeight()-15-1);
			
		}
			
	private:
		
		wsTopLevelWindow *m_pPrevTop;
		
		virtual u32 handleEvent(wsEvent *event)
		{
			u32 type = event->getEventType();
			u32 event_id = event->getEventID();
			u32 id = event->getID();
			u32 result_handled = 0;
			
			// wsWindow *obj = event->getObject();
			
			#if 1
				LOG("dlg::handleEvent(%08x,%d,%d)",type,event_id,id);
			#endif
			
			if (type == EVT_TYPE_BUTTON &&
				event_id == BTN_EVENT_PRESSED)
			{
				if (id == ID_BUTTON_CLOSE)
				{
					printf("hiding dialog\n");
					
					#if 0
						// set the background to gray to prove it only redraws the invalidated area
						wsRect full(0,0,getApplication()->getWidth()-1,getApplication()->getHeight()-1);
					    m_pDC->setClip(full);
					    m_pDC->fillFrame(0,0,full.xe,full.ye,wsDARK_GRAY);
					#endif
					result_handled = 1;
					hide();
				}
			}

			if (!result_handled)
				result_handled = wsTopLevelWindow::handleEvent(event);
			return result_handled;
		}
};




class topWindow : public wsTopLevelWindow
	// The "main" test application window with a
	// bunch of buttons, etc.
{
	public:
		
		topWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
			wsTopLevelWindow(pApp,id,xs,ys,xe,ye,WIN_STYLE_TRANSPARENT),
			m_pDlg(0)
		{
		}
			
	private:
		
		dialogWindow *m_pDlg;
		
		virtual u32 handleEvent(wsEvent *event)
		{
			u32 type = event->getEventType();
			u32 event_id = event->getEventID();
			u32 id = event->getID();
			// wsWindow *obj = event->getObject();
			u32 result_handled = 0;
			
			#if 0
				LOG("handleEvent(%08x,%d,%d)",type,event_id,id);
			#endif
			
			#if 0
				if (event_id == WIN_EVENT_FRAME)
				{
					static int count = 0;
					if (count++ % 30 == 0)
					{
						// LOG("frame count %d",count);
						s_pVU1->updateOnFrameEvent();
						s_pVU2->updateOnFrameEvent();
					}
					result_handled = 1;
				}
				else
			#endif
			
			if (type == EVT_TYPE_BUTTON &&
				event_id == BTN_EVENT_PRESSED)
			{
				if (id == ID_BUTTON1)
				{
					result_handled = 1;
					if (!m_pDlg)
					{
						#define DLG_WIDTH  300
						printf("creating dialog\n");
						m_pDlg = new dialogWindow(this,ID_DLG,
							(getApplication()->getWidth()-DLG_WIDTH)/2,
							20,
							DLG_WIDTH,
							200);
						printf("dlg=%08x\n",(u32)m_pDlg);
						// debugUpdate(1);
					}
					else
					{
						printf("showing dialog\n");
						m_pDlg->show();
					}
				}
				else if (id == ID_MENU1)
				{	
					LOG("ID_MENU1",0);
					menu(ID_MENU1)->popup();
					result_handled = 1;
				}
				else if (id == ID_MENU1_OPTION1)
				{	
					LOG("ID_MENU1_OPTION1",0);

					#if TEST_SOFT_SERIAL
						if (!gp_softSerial)
							gp_softSerial = new softSerial(115200);
						LOG("writing bytes ...",0);
						gp_softSerial->write("the lazy red fox",17);
					#endif
					
					result_handled = 1;
				}
				else if (id == ID_MENU1_OPTION2)
				{	
					LOG("ID_MENU1_OPTION2",0);
					result_handled = 1;
				}
				else if (id == ID_MENU1_OPTION3)
				{	
					LOG("ID_MENU1_OPTION3",0);
					result_handled = 1;
				}
			}
	
			if (!result_handled)
				result_handled = wsTopLevelWindow::handleEvent(event);
			
			return result_handled;
		}
	
};


//---------------------------------------
// wsApplication::Create()
//---------------------------------------
// Called by std_kernel.cpp, this method creates the UI
// which is then "run" via calls to wsApplication::Initialize()
// and wsApplication::timeSlice()

void wsApplication::Create()
{
	LOG("wsApplication::Create(%08x)",this);
	
	s32 width = getWidth();
	s32 height = getHeight();
	
	topWindow *frame = new topWindow(this,ID_WIN_FRAME ,0,0,width-1,height-1);
	LOG("frame=%08x",(u32)frame);
	
	// since all the controls in the top level window will trigger an event on it,
	// they must have unique ids in that context
	
	
	wsWindow *pTitle   = new wsWindow(frame, ID_WIN_TOP,     0,	0, 					  width-1, TOP_MARGIN-1);
	wsWindow *pContent = new wsWindow(frame, ID_WIN_CONTENT, 0,	TOP_MARGIN, 		  width-1, height-BOTTOM_MARGIN); // , 	WIN_STYLE_2D);
	wsWindow *pBottom  = new wsWindow(frame, ID_WIN_BOTTOM,  0,	height-BOTTOM_MARGIN, width-1, height);
	
	pTitle->setBackColor(wsRED);
	pTitle->setForeColor(wsWHITE);
	
	pTitle->setBackColor(wsDARK_BLUE);
	pContent->setBackColor(wsDARK_TURQUOISE);
	pContent->setForeColor(wsBLACK);
	pBottom->setBackColor(wsDARK_BLUE);
	
	new wsButton(pTitle,  ID_BUTTON1, "button1", 	4,   5,   119, 44,  BTN_STYLE_USE_ALTERNATE_COLORS);
	button(ID_BUTTON1)->setAltBackColor(wsBLUE);
	button(ID_BUTTON1)->setAltForeColor(wsWHITE);

	// new wsStaticText(pTitle,  ID_TEXT1,   "text1", 		140, 5,   200, 44);
	// stext(ID_TEXT1)->setFont(wsFont12x16);
	
	wsMenu *pMenu = new wsMenu(pTitle, ID_MENU1, "MENU", width-121, 0, width-1, 49, 120);
	pMenu->addChoice(ID_MENU1_OPTION1,"option1");
	pMenu->addChoice(ID_MENU1_OPTION2,"option2");
	pMenu->addChoice(ID_MENU1_OPTION3,"option3");
	
	#if 1
		s_pVU1 = new awsVuMeter(pTitle,ID_VU1,width-250-1,7,width-130,24,1,12);
		s_pVU2 = new awsVuMeter(pTitle,ID_VU2,width-250-1,26,width-130,43,1,12);
		s_pVU1->setAudioDevice("tdmi",0,0);
		s_pVU2->setAudioDevice("tdmi",0,0);
	#endif
	
	#if 1
		// create the loop track buttons
		
		s32 cwidth = pContent->getWidth();
		s32 cheight= pContent->getHeight();
		
		// allow 4 pixels between buttons
		
		int space = 4;
		
		s32 bwidth = (cwidth - (space * (NUM_LTB_COLS+1))) / NUM_LTB_COLS;
		s32 bheight = (cheight - (space * (NUM_LTB_ROWS+1))) / NUM_LTB_ROWS;
		s32 step_width = (cwidth - space) / NUM_LTB_COLS;
		s32 step_height = (cheight - space) / NUM_LTB_ROWS;
		
		int use_id = ID_LOOP_BUTTON_BASE;
		int use_y = space;
	
		for (u8 row=0; row<NUM_LTB_ROWS; row++)
		{
			int use_x = space;
			for (u8 col=0; col<NUM_LTB_COLS; col++)
			{
				LOG("creating row(%d) col(%d)",row,col);
				new awsLoopTrackButton(
					row,
					col,
					pContent,
					use_id,
					use_x,
					use_y,
					use_x + bwidth-1,
					use_y + bheight-1);
				
				use_id++;
				use_x += step_width;
			}
			use_y += step_height;
			// delay(50);
		}
	#endif
}




