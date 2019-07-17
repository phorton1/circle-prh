// wsWindows - test app.cpp
//
// An application to test the wsWindows UI functionality.
// At the end of the file is wsApplication::Create().
// The calls to Initiatilize() and timeSliceBased() are in kernel.cpp
// Requires USE_UI_SYSTEM to be defined in std_kernel.h

#include <ws/ws.h>
#include <circle/logger.h>
#include <system/std_kernel.h>

#define log_name  "app"

#define WITH_AUDIO_SYSTEM_TEST

#ifdef WITH_AUDIO_SYSTEM_TEST
	#include <audio\Audio.h>
    AudioInputI2S input;
    AudioOutputI2S output;
    AudioControlWM8731 control;
	
	void setup()
	{
        new AudioConnection(input, 0, output, 0);
        new AudioConnection(input, 1, output, 1);
		AudioSystem::initialize(150);
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
		for (u16 i=0; i<=50; i++)
		{
			control.volume(((float)i) / 50.0);
			delay(20);
		}		
	}
	
	void loop()		{}
	
#else	// !WITH_AUDIO_SYSTEM_TEST
	void setup()	{}
	void loop()		{}
#endif

	

#define TOP_MARGIN  50
#define BOTTOM_MARGIN 50


#define ID_WIN_FRAME    1
#define ID_WIN_TOP      10
#define ID_WIN_LEFT     20
#define ID_WIN_RIGHT    30
#define ID_WIN_BOTTOM   40
#define ID_PANEL	   	100

#define ID_BUTTON1    	101
#define ID_BUTTON2    	102
#define ID_BUTTON3    	103
#define ID_BUTTON4    	104
#define ID_BUTTON5    	105

#define ID_TEXT1      	201
#define ID_TEXT2      	202
#define ID_TEXT3      	203
#define ID_TEXT4      	204
#define ID_TEXT5      	205

#define ID_CHECKBOX1  	301
#define ID_CHECKBOX2  	302
#define ID_CHECKBOX3  	303

#define ID_CB_TEXT1   	401
#define ID_CB_TEXT2   	402
#define ID_CB_TEXT3   	403

#define ID_MENU1         		500
#define ID_MENU1_OPTION1		501
#define ID_MENU1_OPTION2        502
#define ID_MENU1_OPTION3		503

#define ID_DLG  1000
#define ID_BUTTON_CLOSE  1001
#define ID_BUTTON_BUG    1002 


#define button(id)   ((wsButton *)findChildByID(id))
#define stext(id)    ((wsStaticText *)findChildByID(id))
#define box(id)      ((wsCheckbox *)findChildByID(id))
#define menu(id)     ((wsMenu *)findChildByID(id))



class dialogWindow : public wsTopLevelWindow
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
			
			new wsButton(this,
				ID_BUTTON_BUG,
				"bug",
				15,
				getHeight()-50,
				95,
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
			
			#if 0
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
				else if (id == ID_BUTTON_BUG)
				{
					static int bug_count = 0;
					CString msg;
					msg.Format("this is bug %d",bug_count++);
					((wsStaticText *)m_pPrevTop->findChildByID(ID_TEXT3))->setText(msg);
				}
			}

			if (!result_handled)
				result_handled = wsTopLevelWindow::handleEvent(event);
			return result_handled;
		}
};




class topWindow : public wsTopLevelWindow
{
	public:
		
		topWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye) :
			wsTopLevelWindow(pApp,id,xs,ys,xe,ye,WIN_STYLE_TRANSPARENT),
			m_pDlg(0)
		{
			m_button1_count = 0;
			m_button2_count = 0;
			m_button3_count = 0;
			m_button4_count = 0;
			m_button5_count = 0;
		}
			
	private:
		
		u16 m_button1_count;
		u16 m_button2_count;
		u16 m_button3_count;
		u16 m_button4_count;
		u16 m_button5_count;
		
		dialogWindow *m_pDlg;
		
		virtual u32 handleEvent(wsEvent *event)
		{
			u32 type = event->getEventType();
			u32 event_id = event->getEventID();
			u32 id = event->getID();
			wsWindow *obj = event->getObject();
			u32 result_handled = 0;
			
			#if DEBUG_TOUCH
				LOG("handleEvent(%08x,%d,%d)",type,event_id,id);
			#endif
			
			if (type == EVT_TYPE_BUTTON &&
				event_id == BTN_EVENT_PRESSED)
			{
				if (id == ID_BUTTON1)
				{
					result_handled = 1;
					m_button1_count++;
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
				else if (id == ID_BUTTON2)
				{
					m_button2_count++;
					wsButton *button = (wsButton *) event->getObject();
					stext(ID_TEXT2)->setText(
						button->isPressed() ? "DOWN" : "UP");
					result_handled = 1;
				}
				else if (id == ID_BUTTON3)
				{
					m_button3_count++;
					CString string;
					string.Format("text3 - %d",m_button3_count);
					stext(ID_TEXT3)->setText(string);
					result_handled = 1;
				}
				else if (id == ID_BUTTON4)
				{
					m_button4_count++;
					if (m_button4_count > 31) m_button4_count = 0;
					
					CString string;
					string.Format("text4 - clicked 0x%02x",m_button4_count);
					stext(ID_TEXT4)->setText(string);
					button(ID_BUTTON5)->setDragConstraint(m_button4_count);
					result_handled = 1;
					
					#ifdef WITH_AUDIO_SYSTEM_TEST
						#define S32_MAX  ((s32)0x7fffffff)
						float float_value = ((float)m_button4_count)/10.00;
						s32 s32_value = (float)(float_value * S32_MAX);
						LOG("float_value=%0.02f  s32_value=%d",float_value,s32_value);
						CCoreTask::Get()->handleEvent(new systemEvent(
							EVENT_TYPE_AUDIO_CONTROL,
							EVENT_ID_ALL,
							s32_value));
					#endif

				}
				else if (id == ID_BUTTON5)
				{
					m_button5_count++;
					CString string;
					string.Format("text5 - clicked %d",m_button5_count);
					stext(ID_TEXT5)->setText(string);
					result_handled = 1;
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
			else if (type == EVT_TYPE_CHECKBOX &&
					 event_id == CHB_EVENT_VALUE_CHANGED)
			{
				if (id == ID_CHECKBOX2)
				{
					wsCheckbox *box = (wsCheckbox *) obj;
					stext(ID_CB_TEXT2)->setText(box->isChecked() ? "two checked" : "two not checked");
					result_handled = 1;
				}
			}
			else if (type == EVT_TYPE_WINDOW &&
					 event_id == WIN_EVENT_LONG_CLICK)
			{
				if (id == ID_BUTTON4)
				{
					CString string;
					m_button5_count++;
					if (m_button4_count > 31) m_button4_count = 0;
					string.Format("text4 - long clicked 0x%02x",m_button4_count);
					stext(ID_TEXT4)->setText(string);
					button(ID_BUTTON5)->setDragConstraint(m_button4_count);
					result_handled = 1;
				}
			}
	
			if (!result_handled)
				result_handled = wsTopLevelWindow::handleEvent(event);
			
			return result_handled;
		}
	
};


void wsApplication::Create()
{
	LOG("wsApplication::Create(%08x)",this);
	
	s32 width = getWidth();
	s32 height = getHeight();
	
	topWindow *frame = new topWindow(this,ID_WIN_FRAME ,0,0,width-1,height-1);
	LOG("frame=%08x",(u32)frame);
	
	// since all the controls in the top level window will trigger an event on it,
	// they must have unique ids in that context
	
	
	wsWindow *pTitle  = new wsWindow(frame, ID_WIN_TOP,    0, 		0, width-1, 			TOP_MARGIN-1);
	wsWindow *pLeft   = new wsWindow(frame, ID_WIN_LEFT,   0, 		TOP_MARGIN, 			width/2-1,		height-BOTTOM_MARGIN-1, 	WIN_STYLE_2D);
	wsWindow *pRight  = new wsWindow(frame, ID_WIN_RIGHT,  width/2, TOP_MARGIN, 			width-1, 		height-BOTTOM_MARGIN-1, 	WIN_STYLE_2D);
	wsWindow *pStatus = new wsWindow(frame, ID_WIN_BOTTOM, 0, 		height-BOTTOM_MARGIN, 	width-1, 		height-1, 					WIN_STYLE_3D);
	
	// app.setBackColor(wsBLACK);
	// app.setForeColor(wsGRAY);
	
	pTitle->setBackColor(wsRED);
	pTitle->setForeColor(wsWHITE);
	
	pLeft->setBackColor(wsDARK_GREEN);
	pLeft->setForeColor(wsMAGENTA);
	
	pRight->setBackColor(wsBLUE);
	pRight->setForeColor(wsYELLOW);
	
	new wsButton		(pTitle,  ID_BUTTON1, "button1", 	4,   5,   119, 44,  BTN_STYLE_USE_ALTERNATE_COLORS);
	new wsStaticText	(pTitle,  ID_TEXT1,   "text1", 		150, 5,   360, 44);
	stext(ID_TEXT1)->setFont(wsFont12x16);
	button(ID_BUTTON1)->setAltBackColor(wsBLUE);
	button(ID_BUTTON1)->setAltForeColor(wsWHITE);
	
	wsMenu *pMenu = new wsMenu(pTitle, ID_MENU1, "MENU", width-121, 0, width-1, 49, 120);
	pMenu->addChoice(ID_MENU1_OPTION1,"option1");
	pMenu->addChoice(ID_MENU1_OPTION2,"option2");
	pMenu->addChoice(ID_MENU1_OPTION3,"option3");
	
	new wsButton		(pLeft,   ID_BUTTON2, "button2", 	4,   5,   119, 33, BTN_STYLE_3D | BTN_STYLE_TOGGLE_COLORS | BTN_STYLE_TOGGLE_VALUE);
	new wsStaticText	(pLeft,   ID_TEXT2,   "text2", 		130, 5,   360, 33);

	new wsButton		(pRight,  ID_BUTTON3, "button3", 	4,   5,   119, 33, BTN_STYLE_3D, WIN_STYLE_CLICK_REPEAT);
	new wsStaticText	(pRight,  ID_TEXT3,   "text3", 		130, 5,   360, 33);
	
	new wsButton		(pStatus, ID_BUTTON4, "button4", 	4,   5,   119, 40, BTN_STYLE_2D | BTN_STYLE_TOGGLE_COLORS, WIN_STYLE_CLICK_LONG);
	new wsStaticText	(pStatus, ID_TEXT4,   "text4", 		130, 5,   360, 40);

	wsWindow *pPanel = new wsWindow(pLeft, ID_PANEL, 10, 40, pLeft->getWidth()-10-1, pLeft->getHeight()-10-1, WIN_STYLE_2D | WIN_STYLE_TRANSPARENT);
	
	// instantiate the drag button after the other objects that are in the background
	
	new wsStaticText	(pPanel,  ID_TEXT5,   "text5", 		130, 5,   360, 33);
	new wsButton		(pPanel,  ID_BUTTON5, "button5", 	4,   5,   119, 33, BTN_STYLE_2D, WIN_STYLE_DRAG);
	
	new wsCheckbox(pRight, ID_CHECKBOX1, 1,  20,45,  0);
	new wsCheckbox(pRight, ID_CHECKBOX2, 1,  20,75,  CHB_STYLE_2D | CHB_STYLE_TOGGLE_COLORS);
	new wsCheckbox(pRight, ID_CHECKBOX3, 1,  20,105, CHB_STYLE_3D | CHB_STYLE_TOGGLE_COLORS);
	
	new wsStaticText(pRight, ID_CB_TEXT1, "one", 	60,45,	360,70 );
	new wsStaticText(pRight, ID_CB_TEXT2, "two", 	60,75,	360,100 );
	new wsStaticText(pRight, ID_CB_TEXT3, "three", 	60,105,	360,130 );
}




