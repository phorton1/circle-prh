//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsNullApp_h
#define _wsNullApp_h

#include "wsApp.h"

// this file should ONLY be included in the main CPP file
// It provides a wsApplication::Create() method that does nothing.

void wsApplication::Create()  {}


#endif  // !_wsNullApp_h
