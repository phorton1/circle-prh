//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsFont_h
#define _wsFont_h

#include <circle/types.h>

typedef struct
{
   unsigned char* p;
   s16 char_width;
   s16 char_height;
   u16 start_char;
   u16 end_char;
} wsFont;

// all pointers exist, but may be null
// see wsFonts.cpp for definition of valid fonts

extern const wsFont *wsFont4x6;
extern const wsFont *wsFont5x8;
extern const wsFont *wsFont5x12;
extern const wsFont *wsFont6x8;
extern const wsFont *wsFont6x10;
extern const wsFont *wsFont7x12;
extern const wsFont *wsFont8x8;
extern const wsFont *wsFont8x12;
extern const wsFont *wsFont8x14;
extern const wsFont *wsFont10x16;
extern const wsFont *wsFont12x16;
extern const wsFont *wsFont12x20;
extern const wsFont *wsFont16x26;
extern const wsFont *wsFont22x36;
extern const wsFont *wsFont24x40;
extern const wsFont *wsFont32x53;


#endif  // !_wsFont_h