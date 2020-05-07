// 08-LinkerTest2 (shell.cpp)
//
// This is a prototype operating system UI shell.
//
// It currently must do some work that I expect to become part of the
// std_kernel, including initializing the file system, and the actual
// program loader.  I hope to keep the UI separate from the kernel.
//
// I'd like the kernel to also present an API that can work
// with the teensyPi.ino and console.pm programs (and/or maybe a separate
// perl wxWidgets UI that somehow communicates with the console box),
// along with a UI like my perl FileManager, that would allow one to
// generically manipulate the files on the SD card through the serial
// and/or ethernet ports.
//
// The shell itself could also be a "client" to a server on the host
// machine, and/or have it's own File Manager UI (also hopefully
// with the UI separated from the kernel)
//
// For now you have to place the test programs on the SD card manually.

#include <ws/ws.h>
#include <circle/logger.h>
#include <system/std_kernel.h>
#include "loader.h"


#define log_name  "shell"

	
//------------------------------------------------------------
// define the layout, ids, windows, and objects of the UI
//------------------------------------------------------------

#define TOP_MARGIN  50

#define ID_WIN_FRAME    1
#define ID_WIN_TITLE    10
#define ID_WIN_CONTENT  20

#define BUTTON_EXIT     110
#define BUTTON_LOAD1    120
#define BUTTON_LOAD2    130


wsWindow *s_pContentWindow = 0;
	// EXTERN referred to by test programs


class frameWindow : public wsTopLevelWindow
	// The operating system shell
{
	public:
		
		frameWindow(wsApplication *pApp, u16 id, s32 xe, s32 ye) :
			wsTopLevelWindow(pApp,id,0,0,xe,ye),
			m_loader(CCoreTask::Get()->GetFileSystem())
		{
			this->setBackColor(wsRED);
			this->setForeColor(wsWHITE);

			new wsButton(this,  BUTTON_EXIT, "EXIT", 	   4,  5, 119, 44, BTN_STYLE_USE_ALTERNATE_COLORS);
			new wsButton(this,  BUTTON_LOAD1,"program1", 144,  5, 259, 44, BTN_STYLE_USE_ALTERNATE_COLORS);
			new wsButton(this,  BUTTON_LOAD2,"program2", 284,  5, 399, 44, BTN_STYLE_USE_ALTERNATE_COLORS);
			
			s_pContentWindow  = new wsWindow(this, ID_WIN_CONTENT, 0, TOP_MARGIN, xe, ye);
			LOG("created Window(%X) content=%X",this,s_pContentWindow);
			s_pContentWindow->setBackColor(wsDARK_GREEN);
			s_pContentWindow->setForeColor(wsBLACK);
		}
			
	private:
		
		CLoader   m_loader;
			// this will move to std_kernel
			

		virtual u32 handleEvent(wsEvent *event)
		{
			u32 type = event->getEventType();
			u32 event_id = event->getEventID();
			u32 id = event->getID();
			
			LOG("handleEvent(%08x,%d,%d)",type,event_id,id);

			// if there is a sub-window occupying the content area
			// pass the event to it to see if can handle it ..
			
			wsWindow *pActiveWindow = s_pContentWindow->getFirstChild();
			u32 result_handled = pActiveWindow ?
				pActiveWindow->handleEvent(event) : 0;

			// otherwise handle "frame" events
			
			if (!result_handled &&
				type == EVT_TYPE_BUTTON &&
				event_id == EVENT_CLICK)
			{
				if (id == BUTTON_EXIT)
				{
					result_handled = 1;
					LOG("BUTTON_EXIT",0);
					printf("calling unloadProgram()\n");
					m_loader.unloadProgram();
					printf("back from unloadProgram()\n");
				}
				else if (id == BUTTON_LOAD1)
				{
					result_handled = 1;
					LOG("BUTTON_LOAD1",0);
					printf("Loading program1.img\n");
					int i = m_loader.loadProgram("program1.img");
					LOG("loadProgram() returned 0x%08x",i);
				}
				else if (id == BUTTON_LOAD2)
				{
					result_handled = 1;
					LOG("BUTTON_LOAD2",0);
					printf("Loading program2.img\n");
					int i = m_loader.loadProgram("program2.img");
					LOG("loadProgram() returned 0x%08x",i);
				}
			}
	
			// and otherwise, pass it to the topLevelWindow parent class
			// for generic handling
			
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

static frameWindow *g_pFrame = 0;
	// not used


void wsApplication::Create()
{
	LOG("wsApplication::Create(%08x)",this);
	
	s32 width = getWidth();
	s32 height = getHeight();
	
	g_pFrame = new frameWindow(this,ID_WIN_FRAME ,width-1,height-1);
	LOG("frame=%08x",(u32)g_pFrame);
}




