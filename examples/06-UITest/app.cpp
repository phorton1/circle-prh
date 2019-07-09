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
		
		dialogWindow(wsApplication *pApp, u16 id, u16 x, u16 y, u16 width, u16 height) :
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
				
			wsStaticText *t =
			new wsStaticText(this,
				0, msg,
				10, 10, getWidth()-10-1, getHeight()-60-1);
			t->setAlign(ALIGN_TOP_LEFT);
			t->setBackColor(wsPURPLE);
			
			new wsButton(this,
				ID_BUTTON_CLOSE,
				"close",
				getWidth()-95,
				getHeight()-50,
				getWidth()-15-1,
				getHeight()-15-1,
				BTN_STYLE_3D);
		}
			
	private:
		
		virtual u32 handleEvent(wsEvent *event)
		{
			u32 type = event->getEventType();
			u32 event_id = event->getEventID();
			u32 id = event->getID();
			// wsWindow *obj = event->getObject();
			
			LOG("dlg::handleEvent(%08x,%d,%d)",type,event_id,id);
			
			if (type == EVT_TYPE_BUTTON &&
				event_id == BTN_EVENT_PRESSED)
			{
				if (id == ID_BUTTON_CLOSE)
				{
					printf("hiding dialog\n");
					wsRect full(0,0,getApplication()->getWidth()-1,getApplication()->getHeight()-1);
					
					// set the background to gray to prove it only redraws the invalidated area
					//
					//    m_pDC->setClip(full);
					//    m_pDC->fillFrame(0,0,full.xe,full.ye,wsDARK_GRAY);
					
					hide();
				}
			}
			return 0;
		}
};




#define ID_BUTTON1    101
#define ID_BUTTON5    105
#define ID_CHECKBOX2  205
#define ID_TEXT6      306

class topWindow : public wsTopLevelWindow
{
	public:
		
		topWindow(wsApplication *pApp, u16 id, u16 xs, u16 ys, u16 xe, u16 ye) :
			wsTopLevelWindow(pApp,id,xs,ys,xe,ye,WIN_STYLE_TRANSPARENT),
			m_pDlg(0) {}
			
	private:
		
		dialogWindow *m_pDlg;
		
		virtual u32 handleEvent(wsEvent *event)
		{
			u32 type = event->getEventType();
			u32 event_id = event->getEventID();
			u32 id = event->getID();
			wsWindow *obj = event->getObject();
			
			LOG("handleEvent(%08x,%d,%d)",type,event_id,id);
			
			if (type == EVT_TYPE_BUTTON &&
				event_id == BTN_EVENT_PRESSED)
			{
				if (id == ID_BUTTON5)
				{
					wsButton *button = (wsButton *) event->getObject();
					findChildByID(ID_TEXT6)->setText(
						button->isPressed() ? "DOWN" : "UP");
				}
				else if (id == ID_BUTTON1)
				{
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
						// printf("dlg=%08x\n",(u32)m_pDlg);
					}
					else
					{
						printf("showing dialog\n");
						m_pDlg->show();
					}
				}
			}
			else if (type == EVT_TYPE_CHECKBOX &&
					 event_id == CHB_EVENT_VALUE_CHANGED &&
					 id == ID_CHECKBOX2)
			{
				wsCheckbox *box = (wsCheckbox *) obj;
				box->setText(box->isChecked() ? "two checked" : "two not checked");
			}
			return 0;
		}
};


void wsApplication::Create()
{
	LOG("wsApplication::Create(%08x)",this);
	
	u16 width = getWidth();
	u16 height = getHeight();
	
	topWindow *frame = new topWindow(this,1,0,0,width-1,height-1);
	// printf("frame=%08x\n",(u32)frame);
	
	// since all the controls in the top level window will trigger an event on it,
	// they must have unique ids in that context
	
	u16 id=1;
	
	wsWindow *pTitle  = new wsWindow(frame, id++, 0, 		0, width-1, 			TOP_MARGIN-1);
	wsWindow *pLeft   = new wsWindow(frame, id++, 0, 		TOP_MARGIN, 			width/2-1,		height-BOTTOM_MARGIN-1, 	WIN_STYLE_2D);
	wsWindow *pRight  = new wsWindow(frame, id++, width/2, 	TOP_MARGIN, 			width-1, 		height-BOTTOM_MARGIN-1, 	WIN_STYLE_2D);
	wsWindow *pStatus = new wsWindow(frame, id++, 0, 		height-BOTTOM_MARGIN, 	width-1, 		height-1, 					WIN_STYLE_3D);
	
	// app.setBackColor(wsBLACK);
	// app.setForeColor(wsGRAY);
	
	pTitle->setBackColor(wsRED);
	pTitle->setForeColor(wsWHITE);
	
	pLeft->setBackColor(wsGREEN);
	pLeft->setForeColor(wsMAGENTA);
	
	pRight->setBackColor(wsBLUE);
	pRight->setForeColor(wsYELLOW);
	
	wsButton *pMainButton =
	new wsButton		(pTitle,  ID_BUTTON1, "button1", 	4,   5,   119, 44, BTN_STYLE_USE_ALTERNATE_COLORS);
	new wsStaticText	(pTitle,  id++, 	  "text1", 		130, 5,   199, 44);
	pMainButton->setAltBackColor(wsBLUE);
	pMainButton->setAltForeColor(wsWHITE);
	
	new wsButton		(pLeft,   id++, "button2", 	4,   5,   119, 33, BTN_STYLE_3D | BTN_STYLE_TOGGLE_COLORS);
	new wsStaticText	(pLeft,   id++, "text2", 		130, 5,   199, 33);

	new wsButton		(pRight,  id++, "button3", 	4,   5,   119, 33, BTN_STYLE_3D);
	new wsStaticText	(pRight,  id++, "text3", 		130, 5,   199, 33);

	new wsButton		(pStatus, id++, "button4", 	4,   5,   119, 40, BTN_STYLE_3D);
	new wsStaticText	(pStatus, id++, "text4", 		130, 5,   199, 40);

	wsWindow *pPanel = new wsWindow(pLeft, 5, 10, 40, pLeft->getWidth()-10-1, pLeft->getHeight()-10-1, WIN_STYLE_2D | WIN_STYLE_TRANSPARENT);
	
	new wsButton		(pPanel,  ID_BUTTON5, "button5", 	4,   5,   119, 33, BTN_STYLE_3D | BTN_STYLE_TOGGLE_COLORS | BTN_STYLE_TOGGLE_VALUE);
	new wsStaticText	(pPanel,  ID_TEXT6,   "text6", 		130, 5,   199, 33);
	
	wsCheckbox *cb1 = new wsCheckbox(pRight, id++, 1,  20,45,  150,74,   0);
	wsCheckbox *cb2 = new wsCheckbox(pRight, ID_CHECKBOX2, 1,  20,75,  150,104,  CHB_STYLE_2D | CHB_STYLE_TOGGLE_COLORS);
	wsCheckbox *cb3 = new wsCheckbox(pRight, id++, 1,  20,105, 150,134,  CHB_STYLE_3D | CHB_STYLE_TOGGLE_COLORS);
	
	cb1->setText("one");
	cb2->setText("two");
	cb3->setText("three");
}




