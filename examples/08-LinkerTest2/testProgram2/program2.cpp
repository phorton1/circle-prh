// program2.cpp
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
// Circle Squared - Copyright (C) 2019 Patrick Horton

#include <system/std_kernel.h>
#include <ws/ws.h>
#include <circle/util.h>

#define log_name "program2"

extern wsWindow *s_pContentWindow;
	// in shell.cpp
	

#define ID_MY_WINDOW   2567
#define ID_MY_BUTTON   2767


class myWindow;				// forward
myWindow *s_myWindow = 0;


class myWindow : public wsWindow
	// an example window with event handling
{
	public:
		
		myWindow() :
			wsWindow(
				s_pContentWindow,
				ID_MY_WINDOW,
				0,0,
				s_pContentWindow->getClientWidth()-1,
				s_pContentWindow->getClientHeight())
		{
			LOG("myWindow(%X) ctor content=%X",this,s_pContentWindow);
			this->setBackColor(wsBLUE);
			this->setForeColor(wsWHITE);
			new wsButton(this, ID_MY_BUTTON, "my button", 10, 10, 120, 45, BTN_STYLE_USE_ALTERNATE_COLORS);
			s_myWindow = this;
		}
			
	private:
		
		virtual u32 handleEvent(wsEvent *event)
			// this regular window is called with an event as if it
			// were a top level window. 
		{
			u32 type = event->getEventType();
			u32 event_id = event->getEventID();
			u32 id = event->getID();
			u32 result_handled = 0;

			LOG("handleEvent(%08x,%d,%d)",type,event_id,id);
			
			if (type == EVT_TYPE_BUTTON &&
				event_id == BTN_EVENT_PRESSED && 
				id == ID_MY_BUTTON)
			{
				result_handled = 1;
				LOG("my Button pressed",0);
				
				static int btn_counter = 0;
				CString fmt;
				fmt.Format("my btn %d",btn_counter++);
				((wsButton *)findChildByID(ID_MY_BUTTON))->setText(fmt);				
			}
	
			return result_handled;
		}
	
};




//---------------------------------------
// boilerplate template program code
//---------------------------------------

void createWindows()
{
	new myWindow();
}


void onUnload()
{
	LOG("onUnload %s",log_name);
	delete s_myWindow;
	s_myWindow = 0;
}


extern "C"
{
	extern int _end;
	extern int __bss_start;
	
	u32 onLoad()
		// the onLoad method, which can be called at the origin of the program memory,
		// returns the address of the onUnload method() which can be anywhere
	{
		u32 end = (u32) &_end;		
		u32 bss = (u32) &__bss_start;		
		LOG("onLoad bss=%X end=%X",bss,end);
		memset((u8*)bss,0,end-bss+1);
		
		createWindows();
		return (u32) &onUnload;
	}
}