//
// wsWindows
//
// A event driven windowing system that kind of combines uGUI and wxWindows.
// Written for the rPi Circle bare metal C++ libraries.

#ifndef _wsColor_h
#define _wsColor_h
#include <circle/types.h>


// #define WS_COLOR_DEPTH_32

#ifdef WS_COLOR_DEPTH_32
	typedef u32 wsColor;
#else
	typedef u16 wsColor;
#endif


#ifndef WS_COLOR_DEPTH_32
    #define wsMAROON                       0x8000
    #define wsDARK_RED                     0x8800
    #define wsBROWN                        0xA145
    #define wsFIREBRICK                    0xB104
    #define wsCRIMSON                      0xD8A7
    #define wsRED                          0xF800
    #define wsTOMATO                       0xFB09
    #define wsCORAL                        0xFBEA
    #define wsINDIAN_RED                   0xCAEB
    #define wsLIGHT_CORAL                  0xEC10
    #define wsDARK_SALMON                  0xE4AF
    #define wsSALMON                       0xF40E
    #define wsLIGHT_SALMON                 0xFD0F
    #define wsORANGE_RED                   0xFA20
    #define wsDARK_ORANGE                  0xFC60
    #define wsORANGE                       0xFD20
    #define wsGOLD                         0xFEA0
    #define wsDARK_GOLDEN_ROD              0xB421
    #define wsGOLDEN_ROD                   0xDD24
    #define wsPALE_GOLDEN_ROD              0xEF35
    #define wsDARK_KHAKI                   0xBDAD
    #define wsKHAKI                        0xEF31
    #define wsOLIVE                        0x8400
    #define wsYELLOW                       0xFFE0
    #define wsYELLOW_GREEN                 0x9E66
    #define wsDARK_OLIVE_GREEN             0x5346
    #define wsOLIVE_DRAB                   0x6C64
    #define wsLAWN_GREEN                   0x7FC0
    #define wsCHART_REUSE                  0x7FE0
    #define wsGREEN_YELLOW                 0xAFE6
    #define wsDARK_GREEN                   0x0320
    #define wsGREEN                        0x07E0
    #define wsFOREST_GREEN                 0x2444
    #define wsLIME                         0x07E0
    #define wsLIME_GREEN                   0x3666
    #define wsLIGHT_GREEN                  0x9772
    #define wsPALE_GREEN                   0x97D2
    #define wsDARK_SEA_GREEN               0x8DD1
    #define wsMEDIUM_SPRING_GREEN          0x07D3
    #define wsSPRING_GREEN                 0x07EF
    #define wsSEA_GREEN                    0x344B
    #define wsMEDIUM_AQUA_MARINE           0x6675
    #define wsMEDIUM_SEA_GREEN             0x3D8E
    #define wsLIGHT_SEA_GREEN              0x2595
    #define wsDARK_SLATE_GRAY              0x328A
    #define wsTEAL                         0x0410
    #define wsDARK_CYAN                    0x0451
    #define wsAQUA                         0x07FF
    #define wsCYAN                         0x07FF
    #define wsLIGHT_CYAN                   0xDFFF
    #define wsDARK_TURQUOISE               0x0679
    #define wsTURQUOISE                    0x46F9
    #define wsMEDIUM_TURQUOISE             0x4E99
    #define wsPALE_TURQUOISE               0xAF7D
    #define wsAQUA_MARINE                  0x7FFA
    #define wsPOWDER_BLUE                  0xAEFC
    #define wsCADET_BLUE                   0x64F3
    #define wsSTEEL_BLUE                   0x4C16
    #define wsCORN_FLOWER_BLUE             0x64BD
    #define wsDEEP_SKY_BLUE                0x05FF
    #define wsDODGER_BLUE                  0x249F
    #define wsLIGHT_BLUE                   0xAEBC
    #define wsSKY_BLUE                     0x867D
    #define wsLIGHT_SKY_BLUE               0x867E
    #define wsMIDNIGHT_BLUE                0x18CE
    #define wsNAVY                         0x0010
    #define wsDARK_BLUE                    0x0011
    #define wsMEDIUM_BLUE                  0x0019
    #define wsBLUE                         0x001F
    #define wsROYAL_BLUE                   0x435B
    #define wsBLUE_VIOLET                  0x897B
    #define wsINDIGO                       0x4810
    #define wsDARK_SLATE_BLUE              0x49F1
    #define wsSLATE_BLUE                   0x6AD9
    #define wsMEDIUM_SLATE_BLUE            0x7B5D
    #define wsMEDIUM_PURPLE                0x939B
    #define wsDARK_MAGENTA                 0x8811
    #define wsDARK_VIOLET                  0x901A
    #define wsDARK_ORCHID                  0x9999
    #define wsMEDIUM_ORCHID                0xBABA
    #define wsPURPLE                       0x8010
    #define wsTHISTLE                      0xD5FA
    #define wsPLUM                         0xDD1B
    #define wsVIOLET                       0xEC1D
    #define wsMAGENTA                      0xF81F
    #define wsORCHID                       0xDB9A
    #define wsMEDIUM_VIOLET_RED            0xC0B0
    #define wsPALE_VIOLET_RED              0xDB92
    #define wsDEEP_PINK                    0xF8B2
    #define wsHOT_PINK                     0xFB56
    #define wsLIGHT_PINK                   0xFDB7
    #define wsPINK                         0xFDF9
    #define wsANTIQUE_WHITE                0xF75A
    #define wsBEIGE                        0xF7BB
    #define wsBISQUE                       0xFF18
    #define wsBLANCHED_ALMOND              0xFF59
    #define wsWHEAT                        0xF6F6
    #define wsCORN_SILK                    0xFFBB
    #define wsLEMON_CHIFFON                0xFFD9
    #define wsLIGHT_GOLDEN_ROD_YELLOW      0xF7DA
    #define wsLIGHT_YELLOW                 0xFFFB
    #define wsSADDLE_BROWN                 0x8A22
    #define wsSIENNA                       0x9A85
    #define wsCHOCOLATE                    0xD344
    #define wsPERU                         0xCC28
    #define wsSANDY_BROWN                  0xF52C
    #define wsBURLY_WOOD                   0xDDB0
    #define wsTAN                          0xD591
    #define wsROSY_BROWN                   0xBC71
    #define wsMOCCASIN                     0xFF16
    #define wsNAVAJO_WHITE                 0xFEF5
    #define wsPEACH_PUFF                   0xFED6
    #define wsMISTY_ROSE                   0xFF1B
    #define wsLAVENDER_BLUSH               0xFF7E
    #define wsLINEN                        0xF77C
    #define wsOLD_LACE                     0xFFBC
    #define wsPAPAYA_WHIP                  0xFF7A
    #define wsSEA_SHELL                    0xFFBD
    #define wsMINT_CREAM                   0xF7FE
    #define wsSLATE_GRAY                   0x7412
    #define wsLIGHT_SLATE_GRAY             0x7453
    #define wsLIGHT_STEEL_BLUE             0xAE1B
    #define wsLAVENDER                     0xE73E
    #define wsFLORAL_WHITE                 0xFFDD
    #define wsALICE_BLUE                   0xEFBF
    #define wsGHOST_WHITE                  0xF7BF
    #define wsHONEYDEW                     0xEFFD
    #define wsIVORY                        0xFFFD
    #define wsAZURE                        0xEFFF
    #define wsSNOW                         0xFFDE
    #define wsBLACK                        0x0000
    #define wsDIM_GRAY                     0x6B4D
    #define wsGRAY                         0x8410
    #define wsDARK_GRAY                    0xAD55
    #define wsSILVER                       0xBDF7
    #define wsLIGHT_GRAY                   0xD69A
    #define wsGAINSBORO                    0xDEDB
    #define wsWHITE_SMOKE                  0xF7BE
    #define wsWHITE                        0xFFFF
    
#else   // 32 bit colors

    #define  wsMAROON                     0x800000
    #define  wsDARK_RED                   0x8B0000
    #define  wsBROWN                      0xA52A2A
    #define  wsFIREBRICK                  0xB22222
    #define  wsCRIMSON                    0xDC143C
    #define  wsRED                        0xFF0000
    #define  wsTOMATO                     0xFF6347
    #define  wsCORAL                      0xFF7F50
    #define  wsINDIAN_RED                 0xCD5C5C
    #define  wsLIGHT_CORAL                0xF08080
    #define  wsDARK_SALMON                0xE9967A
    #define  wsSALMON                     0xFA8072
    #define  wsLIGHT_SALMON               0xFFA07A
    #define  wsORANGE_RED                 0xFF4500
    #define  wsDARK_ORANGE                0xFF8C00
    #define  wsORANGE                     0xFFA500
    #define  wsGOLD                       0xFFD700
    #define  wsDARK_GOLDEN_ROD            0xB8860B
    #define  wsGOLDEN_ROD                 0xDAA520
    #define  wsPALE_GOLDEN_ROD            0xEEE8AA
    #define  wsDARK_KHAKI                 0xBDB76B
    #define  wsKHAKI                      0xF0E68C
    #define  wsOLIVE                      0x808000
    #define  wsYELLOW                     0xFFFF00
    #define  wsYELLOW_GREEN               0x9ACD32
    #define  wsDARK_OLIVE_GREEN           0x556B2F
    #define  wsOLIVE_DRAB                 0x6B8E23
    #define  wsLAWN_GREEN                 0x7CFC00
    #define  wsCHART_REUSE                0x7FFF00
    #define  wsGREEN_YELLOW               0xADFF2F
    #define  wsDARK_GREEN                 0x006400
    #define  wsGREEN                      0x00FF00
    #define  wsFOREST_GREEN               0x228B22
    #define  wsLIME                       0x00FF00
    #define  wsLIME_GREEN                 0x32CD32
    #define  wsLIGHT_GREEN                0x90EE90
    #define  wsPALE_GREEN                 0x98FB98
    #define  wsDARK_SEA_GREEN             0x8FBC8F
    #define  wsMEDIUM_SPRING_GREEN        0x00FA9A
    #define  wsSPRING_GREEN               0x00FF7F
    #define  wsSEA_GREEN                  0x2E8B57
    #define  wsMEDIUM_AQUA_MARINE         0x66CDAA
    #define  wsMEDIUM_SEA_GREEN           0x3CB371
    #define  wsLIGHT_SEA_GREEN            0x20B2AA
    #define  wsDARK_SLATE_GRAY            0x2F4F4F
    #define  wsTEAL                       0x008080
    #define  wsDARK_CYAN                  0x008B8B
    #define  wsAQUA                       0x00FFFF
    #define  wsCYAN                       0x00FFFF
    #define  wsLIGHT_CYAN                 0xE0FFFF
    #define  wsDARK_TURQUOISE             0x00CED1
    #define  wsTURQUOISE                  0x40E0D0
    #define  wsMEDIUM_TURQUOISE           0x48D1CC
    #define  wsPALE_TURQUOISE             0xAFEEEE
    #define  wsAQUA_MARINE                0x7FFFD4
    #define  wsPOWDER_BLUE                0xB0E0E6
    #define  wsCADET_BLUE                 0x5F9EA0
    #define  wsSTEEL_BLUE                 0x4682B4
    #define  wsCORN_FLOWER_BLUE           0x6495ED
    #define  wsDEEP_SKY_BLUE              0x00BFFF
    #define  wsDODGER_BLUE                0x1E90FF
    #define  wsLIGHT_BLUE                 0xADD8E6
    #define  wsSKY_BLUE                   0x87CEEB
    #define  wsLIGHT_SKY_BLUE             0x87CEFA
    #define  wsMIDNIGHT_BLUE              0x191970
    #define  wsNAVY                       0x000080
    #define  wsDARK_BLUE                  0x00008B
    #define  wsMEDIUM_BLUE                0x0000CD
    #define  wsBLUE                       0x0000FF
    #define  wsROYAL_BLUE                 0x4169E1
    #define  wsBLUE_VIOLET                0x8A2BE2
    #define  wsINDIGO                     0x4B0082
    #define  wsDARK_SLATE_BLUE            0x483D8B
    #define  wsSLATE_BLUE                 0x6A5ACD
    #define  wsMEDIUM_SLATE_BLUE          0x7B68EE
    #define  wsMEDIUM_PURPLE              0x9370DB
    #define  wsDARK_MAGENTA               0x8B008B
    #define  wsDARK_VIOLET                0x9400D3
    #define  wsDARK_ORCHID                0x9932CC
    #define  wsMEDIUM_ORCHID              0xBA55D3
    #define  wsPURPLE                     0x800080
    #define  wsTHISTLE                    0xD8BFD8
    #define  wsPLUM                       0xDDA0DD
    #define  wsVIOLET                     0xEE82EE
    #define  wsMAGENTA                    0xFF00FF
    #define  wsORCHID                     0xDA70D6
    #define  wsMEDIUM_VIOLET_RED          0xC71585
    #define  wsPALE_VIOLET_RED            0xDB7093
    #define  wsDEEP_PINK                  0xFF1493
    #define  wsHOT_PINK                   0xFF69B4
    #define  wsLIGHT_PINK                 0xFFB6C1
    #define  wsPINK                       0xFFC0CB
    #define  wsANTIQUE_WHITE              0xFAEBD7
    #define  wsBEIGE                      0xF5F5DC
    #define  wsBISQUE                     0xFFE4C4
    #define  wsBLANCHED_ALMOND            0xFFEBCD
    #define  wsWHEAT                      0xF5DEB3
    #define  wsCORN_SILK                  0xFFF8DC
    #define  wsLEMON_CHIFFON              0xFFFACD
    #define  wsLIGHT_GOLDEN_ROD_YELLOW    0xFAFAD2
    #define  wsLIGHT_YELLOW               0xFFFFE0
    #define  wsSADDLE_BROWN               0x8B4513
    #define  wsSIENNA                     0xA0522D
    #define  wsCHOCOLATE                  0xD2691E
    #define  wsPERU                       0xCD853F
    #define  wsSANDY_BROWN                0xF4A460
    #define  wsBURLY_WOOD                 0xDEB887
    #define  wsTAN                        0xD2B48C
    #define  wsROSY_BROWN                 0xBC8F8F
    #define  wsMOCCASIN                   0xFFE4B5
    #define  wsNAVAJO_WHITE               0xFFDEAD
    #define  wsPEACH_PUFF                 0xFFDAB9
    #define  wsMISTY_ROSE                 0xFFE4E1
    #define  wsLAVENDER_BLUSH             0xFFF0F5
    #define  wsLINEN                      0xFAF0E6
    #define  wsOLD_LACE                   0xFDF5E6
    #define  wsPAPAYA_WHIP                0xFFEFD5
    #define  wsSEA_SHELL                  0xFFF5EE
    #define  wsMINT_CREAM                 0xF5FFFA
    #define  wsSLATE_GRAY                 0x708090
    #define  wsLIGHT_SLATE_GRAY           0x778899
    #define  wsLIGHT_STEEL_BLUE           0xB0C4DE
    #define  wsLAVENDER                   0xE6E6FA
    #define  wsFLORAL_WHITE               0xFFFAF0
    #define  wsALICE_BLUE                 0xF0F8FF
    #define  wsGHOST_WHITE                0xF8F8FF
    #define  wsHONEYDEW                   0xF0FFF0
    #define  wsIVORY                      0xFFFFF0
    #define  wsAZURE                      0xF0FFFF
    #define  wsSNOW                       0xFFFAFA
    #define  wsBLACK                      0x000000
    #define  wsDIM_GRAY                   0x696969
    #define  wsGRAY                       0x808080
    #define  wsDARK_GRAY                  0xA9A9A9
    #define  wsSILVER                     0xC0C0C0
    #define  wsLIGHT_GRAY                 0xD3D3D3
    #define  wsGAINSBORO                  0xDCDCDC
    #define  wsWHITE_SMOKE                0xF5F5F5
    #define  wsWHITE                      0xFFFFFF
    
#endif


#endif  // !_wsColor_h