//--------------------------------------------------------
// ili9488.cpp
//--------------------------------------------------------
// See ili_base.cpp for most information.
//
// I was not able to get 16 bit MCU RGB565 interface working,
// which would promise a 33% speed improvement.
// As far as I can tell the orange ILI9488 display are
// hardwired to use 18 bit interface with RGB666 colors.


#include "ili9488.h"
#include <circle/logger.h>
// #include <circle/gpiomanager.h>
#include <circle/util.h>
#include <utils/myUtils.h>


#define log_name "ili9848"

#define WIDTH		320
#define HEIGHT		480

#define PIXEL_BYTES 3

#define SPI_READ_FREQ  	1000000
#define SPI_WRITE_FREQ  30000000
    // Write appears to work up to about 40Mhz
	// Read is better at or below 20MHZ
	// 1Mhz is useful with the logic analyzer

#define DEBUG_DEFAULT_VALUES 	1
#define DEBUG_INIT_VALUES		0

#define OUTPUT_TEST_PATTERN		1
	// on by default, the screen displays a distinct pattern
	// of squares to verify basic functionality at boot ...


//---------------------------------------------------------------
// initialization sequence - MOSTLY COMMENTS REALLY
//---------------------------------------------------------------

#define PAULS_INIT		0
	// Init values from Pauls ILI9488_t3.
	// They are not needed with the defaults on my orange devices.
#define WITH_BOGUS		0
	// Paul had what I think are some bogus initialization sequences.
	// They are not needed with the defaults on my orange devices.

#define DLY  255

u8 init_sequence[] =	// additional comments from MSWindows Copilot query
{
#if PAULS_INIT
	16, 0xE0, 0x00,0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F,
		// Gamma Correction Settings (Positive Gamma Control)
		// for positive gamma control. Gamma correction adjusts the
		// display’s brightness and contrast characteristics.
	16, 0XE1, 0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F,
		// Gamma Correction Settings (Negative Gamma Control)
	3, 0XC0, 0x17, 0x15,
		// Power Control 1
		// VREG1OUT(0x17) = 1.25 x 4.00 = 5.0000 for positive gamma
		// VREG2OUT(0x15)  = -1.25 x 3.90 = -4.8750 for negative gamma
	2, 0xC1, 0x41,
		// Power Control 2
		// VGH = VCI x 6; VGL=VCI x 4
	4, 0xC5, 0x00, 0x12, 0x80,
		// VCOM Control 1
		// Programs common voltage
		// 0x00 = NOT using NV memory
		// 0x12 = -1.70313   0x80=VCM_REG_EN
	2, 0xB4, 0x02,
		// Display Inversion Control.
		// 0x02 = "2 dot inversion" wetf that means
	2, 0xE9, 0x00,
		// Set Image Function
		// 0x00 = Disable 24 bit data
#endif
#if 0
	// I am having a problem with my LCD connector which connects
	// the LCD and touch SDO (MISO) pins on this device, although
	// I had no such problems on the 3.2" ILI9431 based devices,
	// nor apprarently on the 3.5" blue rPi ILI9486 device (which
	// I am *this* close to just deciding to use for my project).
	//
	// The xpt2046 on the ILI9488 device does not appear to have enough
	// "umph" to drive the joined SDO pin high enough to be seen by the rPi.

	// Paul Changed the SPI mode from the default to apparently NOT use
	// SDO line, but instead use the "3 wire" interface which multiplexes
	// the SDA line (I think).

	2, 0xB0, 0x80,      // Interface Mode Control

	// 0x80 == use 3 wire (bi-directional SDA, no SDO) I think
 	// and this might have something to do with the XPT2046
	// functionality possibly working (untested) with teensy
	// libraries.

#endif
//	#if USE_16_BIT_COLORS
//		2, 0x3A, 0x56,
//			// prh - RGB interface remains at (6) = 18 bits
//			// MPU interface changed to (5) = 16 bit pixel format
//	#elif 0	// NOT NEEDED as 18 bit mode is my orange devices default value
//		2, 0x3A, 0x66,
//			// Interface Pixel Format (18 bit)
//			// Sets the pixel format to 18 bits per pixel (RGB666).
//			// This determines how color data is interpreted by the display.
//	#endif
#if WITH_BOGUS
	// From Paul's code, these are missing bytes by the ILI9488 doc
	2, 0xB1, 0xA0,
		// Frame rate, 60.76hz
		// looks like it should have 2 bytes by documentation
		// assuming it was following zero, RTNA would be prohibited
		// this probably does nothing, maybe breaks things
	3, 0XB6, 0x02, 0x02,
		// Display Function Control
		// Enable RGB/MCU Interface Control
		// Paul had the two 0x02's as a separate line
		// and even then, it looks like it's supposed to be 4 bytes
		// First 0x02=Interval scan
		// Second 0x02=5 frames
	5, 0xF7, 0xA9, 0x51, 0x2C, 0x82,
		// Adjust Control
		// Fine-tunes display parameters.
		// Looks like it takes 10 bytes of data, not 4
#endif
#if	0	// following were commented out in teensy ILI9488_T3
	4, 0xE8, 0x85, 0x00, 0x78,
		// E8 is not any documented command!
	6, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02,
		// Power Control 6
		// These bytes configure power control settings
	2, 0xF7, 0x20,
		// Adjust Control
		// The value 0x20 likely adjusts settings related to color balance, brightness, or contrast.
	3, 0xEA, 0x00, 0x00,
		// Power Control 1
#endif


	//---------------------------------------------------------------
	// In fact, I am only using this small section of initialization
	//---------------------------------------------------------------
	// to wake up and turn on the display.  My orange devices work
	// with the default 18-bit mode.

#if 1
	// Take the display out of any sleep mode and turn it on
	1, 0x11,	// Sleep OUT
	DLY, 150, 	// wait some time
	1, 0x29,	// Display ON
	DLY, 20,	// wait a bit more
#endif

	0			// end marker

};




//---------------------------------------------------
// code
//---------------------------------------------------

ILI9488::~ILI9488()
{
}



ILI9488::ILI9488(ILISPI_CLASS *pSPI) :
	ILIBASE(WIDTH,HEIGHT,PIXEL_BYTES,pSPI,SPI_WRITE_FREQ,SPI_READ_FREQ)
{
}


boolean ILI9488::Initialize()
{
	// debug the default values

	#if DEBUG_DEFAULT_VALUES
		dbgRead("readID",0x04,3);	// 0x54 0x80 0x66 (id1,is2,id3) below
		dbgRead("errors",0x05,1);
		dbgRead("status",0x09,4);	// documented default 0x00 0x61 0x00 0x00
		dbgRead("power ",0x0A,4);	// 0x10 0x00 0x00 0x00
		dbgRead("madctl",0x0B,1);
		dbgRead("pixels",0x0C,1);	// 0x06
		dbgRead("id1   ",0xDA,1);	// 0x54
		dbgRead("id2   ",0xDB,1);	// 0x80
		dbgRead("id3   ",0xDC,1);	// 0x66
	#endif

    LOG("ILI9488::initialize()",0);

	CTimer::Get()->usDelay(20);
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

	// set my preferred rotation
	// xpt2046 was constructed at rotation(0)

	setRotation(3);

	// post initialization debugging

	#if DEBUG_INIT_VALUES

		#if WITH_TRIGGER_PIN
			m_trigger_pin.Write(0);
			CTimer::Get()->usDelay(5);
		#endif

		dbgRead("readID",0x04,3);
		dbgRead("errors",0x05,1);
		dbgRead("status",0x09,4);
			// after minimal init: 		0x54 0x80 0x00 0x00
			// after full-bogus init:   0x24 0x51 0x00 0x00
		dbgRead("power ",0x0A,4);
			// after minimal init: unchanged
		dbgRead("madctl",0x0B,1);
			// after init: 0xE8 as expected for rotation(3)
		dbgRead("pixels",0x0C,1);
			// after init: 0x6 for 18 bit or 0x5 for 16 bit, as expected
	#endif

    #if OUTPUT_TEST_PATTERN
		distinctivePattern();
    #endif

	return true;
}



// virtual
void ILI9488::color565ToBuf(u16 color, u8 *buf)
	// 18 bit color to 3 byte buf
{
	buf[0] = (color >> 11) & 0x1f;
	buf[0] <<= 3;
	buf[1] = (color >> 5) & 0x3f;
	buf[1] <<= 2;
	buf[2] = color & 0x1f;
	buf[2] <<= 3;
}
