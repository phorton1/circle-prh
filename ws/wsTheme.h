//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsTheme_h
#define _wsTheme_h

#include "wsColor.h"

extern const wsColor defaultWindowBackColor;
extern const wsColor defaultWindowForeColor;
extern const wsColor defaultButtonForeColor;
extern const wsColor defaultButtonReleasedColor;
extern const wsColor defaultButtonPressedColor;

extern const wsColor windowFrameColors[];
extern const wsColor buttonPressedFrameColors[];
extern const wsColor buttonReleasedFrameColors[];
extern const wsColor checkboxPressedFrameColors[];
extern const wsColor checkboxReleasedFrameColors[];

extern const u16 num_checkbox_coords;
extern const u16 checkbox_check_coords[];

#endif   // !_wsTheme_h