//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsDefines_h
#define _wsDefines_h

#include <circle/types.h>
#include <circle/timer.h>

#ifndef delay
#define delay(ms)   CTimer::Get()->MsDelay(ms)
#endif

// alignment


#define ALIGN_H_LEFT                                  (1<<0)
#define ALIGN_H_CENTER                                (1<<1)
#define ALIGN_H_RIGHT                                 (1<<2)
#define ALIGN_V_TOP                                   (1<<3)
#define ALIGN_V_CENTER                                (1<<4)
#define ALIGN_V_BOTTOM                                (1<<5)
#define ALIGN_BOTTOM_RIGHT                            (ALIGN_V_BOTTOM|ALIGN_H_RIGHT)
#define ALIGN_BOTTOM_CENTER                           (ALIGN_V_BOTTOM|ALIGN_H_CENTER)
#define ALIGN_BOTTOM_LEFT                             (ALIGN_V_BOTTOM|ALIGN_H_LEFT)
#define ALIGN_CENTER_RIGHT                            (ALIGN_V_CENTER|ALIGN_H_RIGHT)
#define ALIGN_CENTER                                  (ALIGN_V_CENTER|ALIGN_H_CENTER)
#define ALIGN_CENTER_LEFT                             (ALIGN_V_CENTER|ALIGN_H_LEFT)
#define ALIGN_TOP_RIGHT                               (ALIGN_V_TOP|ALIGN_H_RIGHT)
#define ALIGN_TOP_CENTER                              (ALIGN_V_TOP|ALIGN_H_CENTER)
#define ALIGN_TOP_LEFT                                (ALIGN_V_TOP|ALIGN_H_LEFT)

// forwards

class wsRect;
class wsEvent;
class wsWindow;
class wsApplication;

// types

typedef u8 		wsAlignType;

// inlines

#define setBit(val, mask)		val |= (mask)
#define clearBit(val, mask)	    val &= ~(mask)
#define toggleBit(val, mask)    val ^= (mask)

// touch stuff

#define DEBUG_TOUCH   0

#define TOUCH_DOWN   0x01
#define TOUCH_UP     0x02
#define TOUCH_MOVE   0x04

typedef struct {
	u8  state;
	s32 x;
	s32 y;
	u32 time;

	s32 last_x;
	s32 last_y;
	s32 drag_x;
	s32 drag_y;

	bool event_sent;
	u32 last_time;
	
}  touchState_t;


#endif  // !_wsDefines_h
