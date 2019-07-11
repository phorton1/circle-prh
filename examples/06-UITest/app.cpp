// wsWindows - test app.cpp
//
// An application to test the wsWindows UI functionality.
// At the end of the file is wsApplication::Create().
// The calls to Initiatilize() and timeSliceBased() are in kernel.cpp

#include "kernel.h"
#include <circle/util.h>
#include <circle/types.h>
#include <ws/wsWindow.h>


static const char log_name[] = "kapp";

#define TOP_MARGIN  50
#define BOTTOM_MARGIN 50



#define ID_DLG  1000
#define ID_BUTTON_CLOSE  1

class dialogWindow : public wsTopLevelWindow
{
	public:
		
		dialogWindow(wsApplication *pApp, u16 id, s32 x, s32 y, s32 width, s32 height) :
			wsTopLevelWindow(pApp,id,x,y,x + width-1,y + height-1,WIN_STYLE_3D)
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
		
		virtual u32 handleEvent(wsEvent *event)
		{
			u32 type = event->getEventType();
			u32 event_id = event->getEventID();
			u32 id = event->getID();
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
					
					hide();
				}
			}
			return 0;
		}
};


#define ID_WIN_TOP      1
#define ID_WIN_LEFT     2
#define ID_WIN_RIGHT    3
#define ID_WIN_BOTTOM   4

#define ID_BUTTON1    101
#define ID_BUTTON2    102
#define ID_BUTTON3    103
#define ID_BUTTON4    104
#define ID_BUTTON5    105
#define ID_TEXT1      201
#define ID_TEXT2      202
#define ID_TEXT3      203
#define ID_TEXT4      204
#define ID_TEXT5      205
#define ID_CHECKBOX1  301
#define ID_CHECKBOX2  302
#define ID_CHECKBOX3  303
#define ID_CB_TEXT1   401
#define ID_CB_TEXT2   402
#define ID_CB_TEXT3   403

#define button(id)   ((wsButton *)findChildByID(id))
#define stext(id)     ((wsStaticText *)findChildByID(id))
#define box(id)      ((wsCheckbox *)findChildByID(id))


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
			
			#if DEBUG_TOUCH
				LOG("handleEvent(%08x,%d,%d)",type,event_id,id);
			#endif
			
			if (type == EVT_TYPE_BUTTON &&
				event_id == BTN_EVENT_PRESSED)
			{
				if (id == ID_BUTTON1)
				{
					m_button1_count++;
					if (!m_pDlg)
					{
						#define DLG_WIDTH  300
						wsApplication *pApp = getApplication();
						printf("creating dialog\n");
						m_pDlg = new dialogWindow(pApp,ID_DLG,
							(pApp->getWidth()-DLG_WIDTH)/2,
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
				}
				else if (id == ID_BUTTON3)
				{
					m_button3_count++;
					CString string;
					string.Format("text3 - %d",m_button3_count);
					stext(ID_TEXT3)->setText(string);
				}
				else if (id == ID_BUTTON4)
				{
					m_button4_count++;
					if (m_button4_count > 31) m_button4_count = 0;
					
					CString string;
					string.Format("text4 - clicked 0x%02x",m_button4_count);
					stext(ID_TEXT4)->setText(string);
					button(ID_BUTTON5)->setDragConstraint(m_button4_count);
				}
				else if (id == ID_BUTTON5)
				{
					m_button5_count++;
					CString string;
					string.Format("text5 - clicked %d",m_button5_count);
					stext(ID_TEXT5)->setText(string);
				}
			}
			else if (type == EVT_TYPE_CHECKBOX &&
					 event_id == CHB_EVENT_VALUE_CHANGED)
			{
				if (id == ID_CHECKBOX2)
				{
					wsCheckbox *box = (wsCheckbox *) obj;
					stext(ID_CB_TEXT2)->setText(box->isChecked() ? "two checked" : "two not checked");
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
				}
			}
			return 0;
		}
		
};


void wsApplication::Create()
{
	LOG("wsApplication::Create(%08x)",this);
	
	s32 width = getWidth();
	s32 height = getHeight();
	
	topWindow *frame = new topWindow(this,1,0,0,width-1,height-1);
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
	
	new wsButton		(pLeft,   ID_BUTTON2, "button2", 	4,   5,   119, 33, BTN_STYLE_3D | BTN_STYLE_TOGGLE_COLORS | BTN_STYLE_TOGGLE_VALUE);
	new wsStaticText	(pLeft,   ID_TEXT2,   "text2", 		130, 5,   360, 33);

	new wsButton		(pRight,  ID_BUTTON3, "button3", 	4,   5,   119, 33, BTN_STYLE_3D, WIN_STYLE_CLICK_REPEAT);
	new wsStaticText	(pRight,  ID_TEXT3,   "text3", 		130, 5,   360, 33);
	
	new wsButton		(pStatus, ID_BUTTON4, "button4", 	4,   5,   119, 40, BTN_STYLE_2D | BTN_STYLE_TOGGLE_COLORS, WIN_STYLE_CLICK_LONG);
	new wsStaticText	(pStatus, ID_TEXT4,   "text4", 		130, 5,   360, 40);

	wsWindow *pPanel = new wsWindow(pLeft, 5, 10, 40, pLeft->getWidth()-10-1, pLeft->getHeight()-10-1, WIN_STYLE_2D | WIN_STYLE_TRANSPARENT);
	
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




