//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#include "wsTheme.h"

// #define BIG_X

const u16 checkbox_check_coords[] =
{
#ifdef BIG_X
    0, 0,   1, 0,
    0, 1,
#endif
            1, 1,   2, 1,
    1, 2,   2, 2,   3, 2,
    2, 3,   3, 3,   4, 3,
    3, 4,   4, 4,   5, 4,
    4, 5,   5, 5,   6, 5,
    5, 6,   6, 6,   7, 6, 
    6, 7,   7, 7,   8, 7, 
    7, 8,   8, 8,   9, 8, 
    8, 9,   9, 9,   10,9, 
    9, 10,  10,10,
#ifdef BIG_X
                    11,10,
    10,11,  11,11,

    0, 11,  1,11,     
    0, 10,
#endif
    
            1,10,   2,10,
    1, 9,   2,9,    3,9, 
    2, 8,   3,8,    4,8, 
    3, 7,   4,7,    5,7, 
    4, 6,   
                    7,5, 
    6, 4,   7,4,    8,4, 
    7, 3,   8,3,    9,3, 
    8, 2,   9,2,    10,2, 
    9, 1,  10,1,

#ifdef BIG_X
                    11,1, 
    10,0,  11,0,    
#endif
};


const u16 num_checkbox_coords = sizeof(checkbox_check_coords) / 4;


#ifndef WS_COLOR_DEPTH_32   // 16 bit colors (used)

    const wsColor defaultWindowBackColor = 0xEF7D;
    const wsColor defaultWindowForeColor = 0x0000;
    const wsColor defaultButtonForeColor       = defaultWindowForeColor;
    const wsColor defaultButtonReleasedColor   = defaultWindowBackColor;
    const wsColor defaultButtonPressedColor    = wsLIGHT_GRAY;  // 0xD69A
    
    const wsColor windowFrameColors[] =
    {
        0x632C,
        0x632C,
        0x632C,
        0x632C,
        
        0xFFFF,
        0xFFFF,
        0x6B4D,
        0x6B4D,
        
        0xE71C,
        0xE71C,
        0x9D13,
        0x9D13,
    };
        
    const wsColor buttonPressedFrameColors[] =
    {
        0x632C,
        0x632C,
        0x632C,
        0x632C,
        
        0x9D13,
        0x9D13,
        0x9D13,
        0x9D13,
        
        0xEF7D,
        0xEF7D,
        0xEF7D,
        0xEF7D,
    };
    
    const wsColor buttonReleasedFrameColors[] =
    {
        0x632C,
        0x632C,
        0x632C,
        0x632C,
        
        0xFFFF,
        0xFFFF,
        0x6B4D,
        0x6B4D,
        
        0xE71C,
        0xE71C,
        0x9D13,
        0x9D13,
    };
    
    const wsColor checkboxPressedFrameColors[] =
    {
        0x632C,
        0x632C,
        0x632C,
        0x632C,
        
        0x9D13,
        0x9D13,
        0x9D13,
        0x9D13,
        
        0xEF7D,
        0xEF7D,
        0xEF7D,
        0xEF7D,
        };
        
    const wsColor checkboxReleasedFrameColors[] =
    {
        0x632C,
        0x632C,
        0x632C,
        0x632C,
        
        0xFFFF,
        0xFFFF,
        0x6B4D,
        0x6B4D,
        
        0xE71C,
        0xE71C,
        0x9D13,
        0x9D13,
    };

#else  // 32 bit colors (not used)

    const wsColor defaultWindowBackColor = 0xF0F0F0;
    const wsColor defaultWindowForeColor = 0x000000;
    const wsColor defaultButtonForeColor       = defaultWindowForeColor;
    const wsColor defaultButtonReleasedColor   = defaultWindowBackColor;
    const wsColor defaultButtonPressedColor    = wsLIGHT_GRAY;  // 0xD3D3D3

    const wsColor windowFrameColors[] =
    {
        0x646464,
        0x646464,
        0x646464,
        0x646464,

        0xFFFFFF,
        0xFFFFFF,
        0x696969,
        0x696969,

        0xE3E3E3,
        0xE3E3E3,
        0xA0A0A0,
        0xA0A0A0,
    };
    
    const wsColor buttonPressedFrameColors[] =
    {

        0x646464,
        0x646464,
        0x646464,
        0x646464,

        0xA0A0A0,
        0xA0A0A0,
        0xA0A0A0,
        0xA0A0A0,

        0xF0F0F0,
        0xF0F0F0,
        0xF0F0F0,
        0xF0F0F0,
    };
    
    const wsColor buttonReleasedFrameColors[] =
    {
        0x646464,
        0x646464,
        0x646464,
        0x646464,

        0xFFFFFF,
        0xFFFFFF,
        0x696969,
        0x696969,

        0xE3E3E3,
        0xE3E3E3,
        0xA0A0A0,
        0xA0A0A0,
    };
    
    const wsColor checkboxPressedFrameColors[] =
    {
        0x646464,
        0x646464,
        0x646464,
        0x646464,

        0xA0A0A0,
        0xA0A0A0,
        0xA0A0A0,
        0xA0A0A0,

        0xF0F0F0,
        0xF0F0F0,
        0xF0F0F0,
        0xF0F0F0,
    };
    
    const wsColor checkboxReleasedFrameColors[] =
    {
        0x646464,
        0x646464,
        0x646464,
        0x646464,

        0xFFFFFF,
        0xFFFFFF,
        0x696969,
        0x696969,

        0xE3E3E3,
        0xE3E3E3,
        0xA0A0A0,
        0xA0A0A0,
    };
    
#endif	// 32 bit colors

