// 06-UITest (app.cpp)
//
// This is a "program" to test my wsWindows subsystem.
// It, of course, requires that USE_UI_SYSTEM is defined as 1 in std_kernel.h
// At the end of this file is wsApplication::Create(), which *should* be linked
// before the version in std_empty_ui.
//
// The calls to Initialize() and timeSlice() are in std_kernel.cpp
//
// It makes use of whatever UI device (the HDMI screen, official
// rPI touchscreen, or cheap ili9485/xpt2046 3.5 touchscreen, at
// this time) is defined for use there.


#include <ws/ws.h>
#include <circle/logger.h>
#include <system/std_kernel.h>
#include <utils/myUtils.h>

#define log_name  "app"


//------------------------------------------------------------
// define the layout, ids, windows, and objects of the UI
//------------------------------------------------------------

#define TOP_MARGIN  50
#define BOTTOM_MARGIN 50


#define ID_WIN_FRAME    1
#define ID_WIN_TOP      10
#define ID_WIN_LEFT     20
#define ID_WIN_RIGHT    30
#define ID_WIN_BOTTOM   40
#define ID_PANEL	   	100

#define ID_BUTTON_DLG   	101
#define ID_BUTTON_UP_DOWN   102
#define ID_BUTTON_COUNT    	103
#define ID_BUTTON_MODE    	104
#define ID_BUTTON_DRAG    	105

// #define ID_TEXT1      	201
#define ID_TEXT_UP_DOWN		202
#define ID_TEXT_COUNT		203
#define ID_TEXT_DRAG      	204
// #define ID_TEXT5      	205

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


const char *dragModeToStr(u16 mode)
{
	CString msg;
	msg.Format("mode(%d)",mode);
	if (mode & DRAG_CONSTRAINT_X)
		msg.Append("X ");
	if (mode & DRAG_CONSTRAINT_Y)
		msg.Append("Y ");
	if (mode & DRAG_CONSTRAINT_FIT)
		msg.Append("FIT ");
	if (mode & DRAG_CONSTRAINT_SHOW)
		msg.Append("SHOW ");
	if (mode & DRAG_CONSTRAINT_OBJECT)
		msg.Append("OBJ ");

	static char buf[80];
	strcpy(buf,msg);
	return buf;
}



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
				event_id == EVENT_CLICK)
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
					((wsStaticText *)m_pPrevTop->findChildByID(ID_TEXT_COUNT))->setText(msg);

					// the reason this is questionable is that entire static text control
					// repaints itself, and then later, the dialog again repaints itself.
					// only the portion of the static text control that is really visible
					// should be repainted, which requires complicated clipping regions.
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
			m_button_count = 0;
			m_drag_mode = 0;
		}

	private:

		u16 m_button_count;
		u16 m_drag_mode;

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
				event_id == EVENT_CLICK)
			{
				if (id == ID_BUTTON_DLG)
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
				else if (id == ID_BUTTON_UP_DOWN)
				{
					wsButton *button = (wsButton *) event->getObject();
					stext(ID_TEXT_UP_DOWN)->setText( button->isPressed() ? "DOWN" : "UP");
					result_handled = 1;
				}
				else if (id == ID_BUTTON_COUNT)
				{
					m_button_count++;
					CString string;
					string.Format("count(%d)",m_button_count);
					stext(ID_TEXT_COUNT)->setText(string);
					result_handled = 1;
				}
				else if (id == ID_BUTTON_MODE)
				{
					m_drag_mode++;
					if (m_drag_mode > 31) m_drag_mode = 0;

					CString string = dragModeToStr(m_drag_mode);
					stext(ID_TEXT_DRAG)->setText(string);
					button(ID_BUTTON_DRAG)->setDragConstraint(m_drag_mode);
					result_handled = 1;
				}
				else if (id == ID_BUTTON_DRAG)
				{
					stext(ID_TEXT_DRAG)->setText("CLICKED");
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
					#if USE_XPT2046
						LOG("ui starting tft calibration",0);
						CCoreTask::Get()->GetKernel()->GetXPT2046()->startCalibration();
					#else
						LOG("ID_MENU1_OPTION1",0);
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
			else if (type == EVT_TYPE_CHECKBOX &&
					 event_id == EVENT_VALUE_CHANGED)
			{
				if (id >= ID_CHECKBOX1 && id <= ID_CHECKBOX3)
				{
					wsCheckbox *box = (wsCheckbox *) obj;

					u32 num = id - ID_CHECKBOX1;
					CString msg;
					msg.Format("%s %s",
						num == 2 ? "three" : num == 1 ? "two" : "one",
						box->isChecked() ? "checked" : "off" );

					stext(ID_CB_TEXT1 + num)->setText(msg);
					result_handled = 1;
				}
			}
			else if (type == EVT_TYPE_WINDOW &&
					 event_id == EVENT_LONG_CLICK)
			{
				if (id == ID_BUTTON_MODE)
				{
					stext(ID_TEXT_DRAG)->setText("Drag Mode Reset");
					button(ID_BUTTON_DRAG)->move(4,5);	// restore original position
					m_drag_mode = 0;					// restore drag constraint
					button(ID_BUTTON_DRAG)->setDragConstraint(m_drag_mode);
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

	new wsButton(pTitle,  ID_BUTTON_DLG, "DLG", 	4,   5,   119, 44,  BTN_STYLE_USE_ALTERNATE_COLORS);
	button(ID_BUTTON_DLG)->setAltBackColor(wsBLUE);
	button(ID_BUTTON_DLG)->setAltForeColor(wsWHITE);

	wsMenu *pMenu = new wsMenu(pTitle, ID_MENU1, "MENU", width-121, 0, width-1, 49, 120);

	#if USE_XPT2046
		pMenu->addChoice(ID_MENU1_OPTION1,"calib");
	#else
		pMenu->addChoice(ID_MENU1_OPTION1,"option2");
	#endif
	pMenu->addChoice(ID_MENU1_OPTION2,"option2");
	pMenu->addChoice(ID_MENU1_OPTION3,"option3");


	new wsButton		(pLeft,   ID_BUTTON_UP_DOWN, 	"up_down", 		4,   5,   119, 33, BTN_STYLE_3D | BTN_STYLE_TOGGLE_COLORS | BTN_STYLE_TOGGLE_VALUE);
	new wsStaticText	(pLeft,   ID_TEXT_UP_DOWN,      "up(0)", 		130, 5,   360, 33);

	new wsButton		(pRight,  ID_BUTTON_COUNT, 		"count", 		4,   5,   119, 33, BTN_STYLE_3D, WIN_STYLE_CLICK_REPEAT);
	new wsStaticText	(pRight,  ID_TEXT_COUNT,   		"count(0)", 	130, 5,   360, 33);

	new wsButton		(pStatus, ID_BUTTON_MODE, 		"drag mode", 	4,   5,   119, 40, BTN_STYLE_2D | BTN_STYLE_TOGGLE_COLORS, WIN_STYLE_CLICK_LONG);
	new wsStaticText	(pStatus, ID_TEXT_DRAG,   		"drag_text", 	130, 5,   360, 40);

	wsWindow *pPanel = new wsWindow(pLeft, ID_PANEL, 10, 40, pLeft->getWidth()-10-1, pLeft->getHeight()-10-1, WIN_STYLE_2D | WIN_STYLE_TRANSPARENT);

	// instantiate the drag button after the other objects that are in the background

	new wsButton		(pPanel,  ID_BUTTON_DRAG, "drag", 	4,   5,   119, 33, BTN_STYLE_2D, WIN_STYLE_DRAG);

	new wsCheckbox(pRight, ID_CHECKBOX1, 1,  20,45,  0);
	new wsCheckbox(pRight, ID_CHECKBOX2, 1,  20,75,  CHB_STYLE_2D | CHB_STYLE_TOGGLE_COLORS);
	new wsCheckbox(pRight, ID_CHECKBOX3, 1,  20,105, CHB_STYLE_3D | CHB_STYLE_TOGGLE_COLORS);

	new wsStaticText(pRight, ID_CB_TEXT1, "one", 	60,45,	360,70 );
	new wsStaticText(pRight, ID_CB_TEXT2, "two", 	60,75,	360,100 );
	new wsStaticText(pRight, ID_CB_TEXT3, "three", 	60,105,	360,130 );
}
