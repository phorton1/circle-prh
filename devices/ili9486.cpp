//-----------------------------------------------
// ili9486.cpp - for blue 3.5" rPi 320x480 screen
// see pin notes in ili_base.cpp.
// This device also uses a RESET pin

#include "ili9486.h"
#include <circle/logger.h>
#include <circle/gpiomanager.h>
#include <circle/util.h>
#include <utils/myUtils.h>


#define log_name "ili9846s"

#define PIN_RESET       25

#define WIDTH			320
#define HEIGHT			480
#define PIXEL_BYTES 	2

#define SPI_WRITE_FREQ  26000000
#define SPI_READ_FREQ	1000000
    // it appears to work up to about 40Mhz
    // 26Mhz is an arbitrary choice
	// 1 Mhz is better for logic analyzer


#define OUTPUT_TEST_PATTERN	1
	// on by default, the screen displays a distinct pattern
	// of squares to verify basic functionality at boot ...


//---------------------------------
// init sequence
//---------------------------------

#define DLY  			255

u8 init_sequence[] =
{
//	2, 0xb0, 0x0,	// Interface Mode Control
//	1, 0x11,		// Sleep OUT
//	DLY, 150,
	2, 0x3A, 0x55,	// 16 bits per pixel color
	2, 0x36, 0x48,	// MX, BGR == rotation 0
//	2, 0xC2, 0x44,	// Power Control 3
	// VCOM Control 1
//	5, 0xC5, 0x00, 0x00, 0x00, 0x00,
	// PGAMCTRL(Positive Gamma Control)
	16, 0xE0, 0x0F, 0x1F, 0x1C, 0x0C, 0x0F, 0x08, 0x48,
	    0x98, 0x37, 0x0A, 0x13, 0x04, 0x11, 0x0D, 0x00,
	// NGAMCTRL(Negative Gamma Control)
	16, 0xE1, 0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47,
	    0x75, 0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00,
	// Digital Gamma Control 1
	16, 0xE2, 0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47,
		0x75, 0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00,
	1, 0x11,	// Sleep OUT
	DLY, 150, 	// wait some time
	1, 0x29,	// Display ON
	0			// end marker
};






ILI9846::~ILI9846()
{
}


ILI9846::ILI9846(CSPIMaster *pSPI) :
	ILIBASE(WIDTH,HEIGHT,PIXEL_BYTES,pSPI,SPI_WRITE_FREQ,SPI_READ_FREQ)
{
	m_rotation = 0;
}



// virtual
boolean ILI9846::Initialize()
{
    LOG("initialize",0);
    m_pinCD.Write(1);

    m_pinRESET.Write(1);
    CTimer::Get()->usDelay(1000);
    m_pinRESET.Write(0);
    CTimer::Get()->usDelay(300);
    m_pinRESET.Write(1);
    CTimer::Get()->usDelay(1000);
    m_pinRESET.Write(1);


    u8 *p = init_sequence;
    while (*p)
    {
        u8 len = *p++;
        if (len == DLY)
        {
            CTimer::Get()->MsDelay(*p++);
        }
        else
        {
            u8 cmd = *p++;
            len--;
            writeCommand(cmd,p,len);
            p += len;
            CTimer::Get()->usDelay(20);
        }
    }

	setRotation(3);

    #if OUTPUT_TEST_PATTERN
		distinctivePattern();
    #endif

	return true;
}



// virtual
void ILI9846::color565ToBuf(u16 color, u8 *buf)
	// 18 bit color to 2 byte buf
{
	buf[0] = color >> 8;
	buf[1] = color & 0xff;
}
