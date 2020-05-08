//---------------------------------------
// wsApplication::Create()
//---------------------------------------
// Called by std_kernel.cpp, this method creates the UI
// which is then "run" via calls to wsApplication::Initialize()
// and wsApplication::timeSlice()

#include "LoopWindow.h"
#include <circle/logger.h>
#include <utils/myUtils.h>

#define log_name  "app"


#define ID_WIN_LOOPER    	100


void wsApplication::Create()
{
	LOG("wsApplication::Create(%08x)",this);
	new LoopWindow(this,ID_WIN_LOOPER,0,0,getWidth()-1,getHeight()-1);
}

