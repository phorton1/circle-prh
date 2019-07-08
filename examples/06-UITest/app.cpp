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


void wsApplication::Create()
{
	LOG("wsApplication::Create()",0);
	
	u16 width = getWidth();
	u16 height = getHeight();
	
	wsTopLevelWindow *frame = new wsTopLevelWindow(this,1,0,0,width-1,height-1);
	
	// since all the controls in the top level window will trigger an event on it,
	// they must have unique ids in that context
	
	u16 id=1;
	
	wsWindowBase *pTitle  = new wsWindowBase(frame, id++, 0, 		0, width-1, 			TOP_MARGIN-1);
	wsWindowBase *pLeft   = new wsWindowBase(frame, id++, 0, 		TOP_MARGIN, 			width/2-1,		height-BOTTOM_MARGIN-1, 	WIN_STYLE_2D);
	wsWindowBase *pRight  = new wsWindowBase(frame, id++, width/2, 	TOP_MARGIN, 			width-1, 		height-BOTTOM_MARGIN-1, 	WIN_STYLE_2D);
	wsWindowBase *pStatus = new wsWindowBase(frame, id++, 0, 		height-BOTTOM_MARGIN, 	width-1, 		height-1, 					WIN_STYLE_3D);
	
	// app.setBackColor(wsBLACK);
	// app.setForeColor(wsGRAY);
	
	pTitle->setBackColor(wsRED);
	pTitle->setForeColor(wsWHITE);
	
	pLeft->setBackColor(wsGREEN);
	pLeft->setForeColor(wsMAGENTA);
	
	pRight->setBackColor(wsBLUE);
	pRight->setForeColor(wsYELLOW);
	
	wsButton *pMainButton =
	new wsButton		(pTitle,  id++, "button1", 	4,   5,   119, 44, BTN_STYLE_USE_ALTERNATE_COLORS);
	new wsStaticText	(pTitle,  id++, "text1", 		130, 5,   199, 44);
	pMainButton->setAltBackColor(wsBLUE);
	pMainButton->setAltForeColor(wsWHITE);
	
	new wsButton		(pLeft,   id++, "button2", 	4,   5,   119, 33, BTN_STYLE_3D | BTN_STYLE_TOGGLE_COLORS);
	new wsStaticText	(pLeft,   id++, "text2", 		130, 5,   199, 33);

	new wsButton		(pRight,  id++, "button3", 	4,   5,   119, 33, BTN_STYLE_3D);
	new wsStaticText	(pRight,  id++, "text3", 		130, 5,   199, 33);

	new wsButton		(pStatus, id++, "button4", 	4,   5,   119, 40, BTN_STYLE_3D);
	new wsStaticText	(pStatus, id++, "text4", 		130, 5,   199, 40);

	wsWindowBase *pPanel = new wsWindowBase(pLeft, 5, 10, 40, pLeft->getWidth()-10-1, pLeft->getHeight()-10-1, WIN_STYLE_2D );
	
	new wsButton		(pPanel,  id++, "button5", 	4,   5,   119, 33, BTN_STYLE_3D | BTN_STYLE_TOGGLE_COLORS | BTN_STYLE_TOGGLE_VALUE);
	new wsStaticText	(pPanel,  id++, "text6", 		130, 5,   199, 33);
	
	wsCheckbox *cb0 = new wsCheckbox(pRight, id++, 1,  20,45,  150,74,   0);
	wsCheckbox *cb1 = new wsCheckbox(pRight, id++, 1,  20,75,  150,104,  CHB_STYLE_2D | CHB_STYLE_TOGGLE_COLORS);
	wsCheckbox *cb2 = new wsCheckbox(pRight, id++, 1,  20,105, 150,134,  CHB_STYLE_3D | CHB_STYLE_TOGGLE_COLORS);
	
	cb0->setText("one");
	cb1->setText("two");
	cb2->setText("three");
}




