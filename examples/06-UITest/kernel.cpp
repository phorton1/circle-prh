// kernel.cpp
//
// Based on Circle - A C++ bare metal environment for Raspberry Pi
// which is Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// This code is Copyright (C) 2019, Patrick Horton, no rights reserved.

#include "kernel.h"
#include <circle/util.h>
#include <circle/types.h>
#include <ws/wsWindow.h>


static const char log_name[] = "kernel";


CKernel::CKernel(void) :
	m_Screen(1440, 768),	// 800, 480),
	m_Timer(&m_Interrupt),
	m_Serial(&m_Interrupt, TRUE),
	m_Logger(LogDebug,&m_Timer)
	#if USE_USB
		,m_DWHCI(&m_Interrupt, &m_Timer)
	#endif
	#if USE_ALT_SCREEN
		,m_ili9486(&m_SPI)
		,m_xpt2046(&m_SPI)
	#endif
{
	m_ActLED.Toggle();
}


CKernel::~CKernel (void)
{
}


boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;
	
	if (bOK)
		bOK = m_Screen.Initialize();
	if (bOK)
		bOK = m_Interrupt.Initialize();
	if (bOK)
		bOK = m_Timer.Initialize();
	if (bOK)
		bOK = m_Serial.Initialize(115200);
	if (bOK)
		bOK = m_Logger.Initialize(&m_Serial);
	#if USE_USB
		if (bOK)
			bOK = m_DWHCI.Initialize ();
	#endif

	if (bOK)
	{
		#if USE_ALT_SCREEN
			m_SPI.Initialize();
			m_ili9486.Initialize();
		#else
			m_TouchScreen.Initialize ();
		#endif
	}
	
	return bOK;
}


void CKernel::setupApp()
{
	LOG("setupApp()",0);
	
	CMouseDevice *pMouse = (CMouseDevice *) CDeviceNameService::Get ()->GetDevice ("mouse1", FALSE);
	
	#if USE_ALT_SCREEN
		CScreenDeviceBase *pUseScreen = &m_ili9486;
		CTouchScreenBase  *pTouch = &m_xpt2046;
	#else
		CScreenDeviceBase *pUseScreen = &m_Screen;
		CTouchScreenBase  *pTouch = (CTouchScreenBase *) CDeviceNameService::Get ()->GetDevice ("touch1", FALSE);
	#endif
	
	#define TOP_MARGIN  50
	#define BOTTOM_MARGIN 50
	
	m_app.Initialize(pUseScreen,pTouch,pMouse);
	
	u16 width = m_app.getWidth();
	u16 height = m_app.getHeight();
	
	wsTopLevelWindow *frame = new wsTopLevelWindow(&m_app,1,0,0,width-1,height-1);
	
	wsWindowBase *pTitle  = new wsWindowBase(frame, 1, 0, 			0, width-1, 			TOP_MARGIN-1);
	wsWindowBase *pLeft   = new wsWindowBase(frame, 2, 0, 			TOP_MARGIN, 			width/2-1,		height-BOTTOM_MARGIN-1, 	WIN_STYLE_2D);
	wsWindowBase *pRight  = new wsWindowBase(frame, 3, width/2, 	TOP_MARGIN, 			width-1, 		height-BOTTOM_MARGIN-1, 	WIN_STYLE_2D);
	wsWindowBase *pStatus = new wsWindowBase(frame, 4, 0, 			height-BOTTOM_MARGIN, 	width-1, 		height-1, 					WIN_STYLE_3D);
	
	// app.setBackColor(wsBLACK);
	// app.setForeColor(wsGRAY);
	
	pTitle->setBackColor(wsRED);
	pTitle->setForeColor(wsWHITE);
	
	pLeft->setBackColor(wsGREEN);
	pLeft->setForeColor(wsMAGENTA);
	
	pRight->setBackColor(wsBLUE);
	pRight->setForeColor(wsYELLOW);
	
	new wsButton		(pTitle,  1, "button1", 	4,   5,   119, 44);
	new wsStaticText	(pTitle,  2, "text1", 		130, 5,   199, 44);

	new wsButton		(pLeft,   1, "button2", 	4,   5,   119, 33, BTN_STYLE_3D);
	new wsStaticText	(pLeft,   2, "text2", 		130, 5,   199, 33);

	new wsButton		(pRight,  1, "button3", 	4,   5,   119, 33, BTN_STYLE_3D);
	new wsStaticText	(pRight,  2, "text3", 		130, 5,   199, 33);

	new wsButton		(pStatus, 1, "button4", 	4,   5,   119, 40, BTN_STYLE_3D);
	new wsStaticText	(pStatus, 2, "text4", 		130, 5,   199, 40);

	wsWindowBase *pPanel = new wsWindowBase(pLeft, 5, 10, 40, pLeft->getWidth()-10-1, pLeft->getHeight()-10-1, WIN_STYLE_2D );
	
	new wsButton		(pPanel,  1, "button5", 	4,   5,   119, 33, BTN_STYLE_3D);
	new wsStaticText	(pPanel,  2, "text6", 		130, 5,   199, 33);
	
	wsCheckbox *cb0 = new wsCheckbox(pRight,  3, 1,  20,45,  150,74,   0);
	wsCheckbox *cb1 = new wsCheckbox(pRight,  3, 1,  20,75,  150,104,  CHB_STYLE_2D);
	wsCheckbox *cb2 = new wsCheckbox(pRight,  3, 1,  20,105, 150,134,  CHB_STYLE_3D);
	
	cb0->setText("one");
	cb1->setText("two");
	cb2->setText("three");
				 
	m_app.draw();
	
	// m_app.getDC()->setBackColor(wsPURPLE);
	// m_app.getDC()->setForeColor(wsWHITE);
	// m_app.getDC()->putString(50,50,"this is a test\n1234\nok");	
}



TShutdownMode CKernel::Run(void)
	// calls the CoreTask to do all the work
{
	LOG("06-UITest " __DATE__ " " __TIME__,0);
	setupApp();
	
	while (1)
	{
		m_app.timeSlice();
		CTimer::Get()->MsDelay(1);
	}

	return ShutdownHalt;
}


