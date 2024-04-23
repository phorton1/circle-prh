//---------------------------------------------------------------
// ili9488.cpp
//---------------------------------------------------------------
// Note that on Orange 3.5" 320x480 ili9488 screens, I had
// to redesign the teensyExpression2 lcd_connector and pull
// MISO first from the TOUCH_DO pin, and separate it from the the
// LCD_MISO pin by a 220 ohm resistor in order to be able to
// read from both the ili9488 and xpt2046 on the same SPI bus.
// Otherwise, with previous connector design, the xpt2046 MISO
// signal was pulled down to below 1V by the ili9488 MISO line
// and the xpt2046 did not work.
//
// Note the GetPixel() is not implemented in these devices,
// so the only use of the LCD_MISO pin is for dbgRead() debugging.
// Another alternative is to merely not hook up the LCD_MISO pin.


#include "ili9488.h"
#include <circle/logger.h>
#include <circle/gpiomanager.h>
#include <circle/util.h>
#include <utils/myUtils.h>


#define log_name "ili9488"

#define WIDTH		320
#define HEIGHT		480

// Turn DEBUG_CTOR off if staticically created!

#define DEBUG_CTOR	1

// May want to turn these three off in production use

#define DEBUG_DEFAULT_VALUES 	1
#define DEBUG_INIT_VALUES		1
#define OUTPUT_TEST_PATTERN		1
	// on by default, the screen displays a distinct pattern
	// of squares to verify basic functionality at boot ...


#define SPI_READ_FREQ  	1000000
#define SPI_WRITE_FREQ  30000000
    // Write appears to work up to about 40Mhz
	// Read is better at or below 20MHZ
	// 1Mhz is useful with the logic analyzer

#define USE_16_BIT_COLORS	0
	// I was not able to get 16 bit MCU RGB565 interface working,
	// which would promise a 33% speed improvement.
	// As far as I can tell the orange ILI9488 display are
	// hardwired to use 18 bit interface with RGB666 colors.
	// More testing is warranted.
#if USE_16_BIT_COLORS
	#define PIXEL_BYTES 2
#else
	#define PIXEL_BYTES 3
#endif


// some RGB565 colors for testing

#define C_BLACK     0x0000
#define C_BLUE      0x001F
#define C_GREEN     0x07E0
#define C_RED       0xF800
#define C_YELLOW    0xFFE0
#define C_CYAN      0x07FF
#define C_WHITE     0xFFFF


//---------------------------------------------------------------------------------
// pin  gpioname    fxn             desc
//---------------------------------------------------------------------------------
// The pins for the Orange 3.5" ILI9488 are unchanged from the Blue rPi
// 3.5" 320x480 ILI9486 hat, except that we do not use the RST pin,
// which is pulled up to 5V by a 10K resistor on the lcd_connector.
//
// 18 	gpio24		LCD_RS 	        LCD instruction control, Instruction/Data register selection
//									called 'CD' in my code
// 19 	spi_mosi	LCD_SI/TP_SI 	SPI data input of both LCD & touch panel
// 21 	spi_miso	TP_S0 	        SPI data output of touch panel
// 23 	spi_sclk	LCD_SCK/TP_SCK 	SPI clock for both LCD & touch panel
// 24 	gpio8		LCD_CS 	        LCD chip select (active low)
// 26 	gpio27		TP_CS 	        Touch panel chip select (active low)
//
// #define PIN_CS_TOUCH    7
// #define PIN_CS_LCD      8
// #define PIN_MISO        9
// #define PIN_MOSI        10
// #define PIN_SCLK        11

#define PIN_CD          24
#define DLY  			255



#define PAULS_INIT		0
	// Init values from Pauls ILI9488_t3.
	// They are not needed with the defaults on my orange devices.
#define WITH_BOGUS		0
	// Paul had what I think are some bogus initialization sequences.
	// They are not needed with the defaults on my orange devices.


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
	2, 0xB0, 0x80,      // Interface Mode Control
		// 0x80 == use 3 wire (bi-directional SDA, no SDO)
		// Paul Changed the SPI mode from the default to apparently use the
		// the "3 wire" interface which multiplexes the SDA line.
		// This might have something to do with the XPT2046
		// functionality possibly working (untested) with teensy
		// libraries.
#endif
#if USE_16_BIT_COLORS
	2, 0x3A, 0x56,
		// prh - set RGB interface remains to 16 bits
		// I was not able to get this working (with either 0x56, 0x65, or 0x55)
#elif 0	// 18 bit is the orange devices DEFAULT_VALUE
	2, 0x3A, 0x66,
		// Interface Pixel Format (18 bit)
		// Sets the pixel format to 18 bits per pixel (RGB666).
		// This determines how color data is interpreted by the display.
#endif
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


// In the end I am only using this small section of initialization
// to wake up and turn on the display.  My orange devices work
// with the device defaults

#if 1
	// Take the display out of any sleep mode and turn it on
	1, 0x11,	// Sleep OUT
	DLY, 150, 	// wait some time
	1, 0x29,	// Display ON
	DLY, 20,	// wait a bit more
#endif

	0			// end marker

};



ILI9488::~ILI9488()
{
}



ILI9488::ILI9488(CSPIMaster *pSPI) :
    m_pSPI(pSPI),
    m_pinCD(PIN_CD,GPIOModeOutput)
	#if WITH_TRIGGER_PIN
		,m_trigger_pin(WITH_TRIGGER_PIN,GPIOModeOutput)
	#endif
{
	#if DEBUG_CTOR
		LOG("ctor WITH_TRIGGER_PIN(%d)",WITH_TRIGGER_PIN);
	#endif
	m_rotation = 0;
	m_pinCD.Write(1);
	#if WITH_TRIGGER_PIN
		m_trigger_pin.Write(1);
	#endif
}


// virtual
void ILI9488::InitializeUI(void *pUI, DriverRegisterFxn registerFxn)
{
	LOG("InitializeUI()",0);
	registerFxn(pUI, this, SCREEN_OPT_FILL_FRAME, (void *) staticFillFrame );
	registerFxn(pUI, this, SCREEN_OPT_FILL_AREA, (void *) staticFillArea );
}


void ILI9488::write(u8 *data, u16 len)
{
    CTimer::Get()->usDelay(1);
	while (len--)
	{
		m_pSPI->SetClock(SPI_WRITE_FREQ);
		m_pSPI->WriteRead(0,data++,0,1);
		CTimer::Get()->usDelay(1);
	}
}


void ILI9488::writeCommand(u8 command, u8 *data, u16 len)
{
    m_pinCD.Write(0);
    write(&command,1);
    m_pinCD.Write(1);
    if (len)
		write(data,len);
}



void ILI9488::dbgRead(const char *what, u8 command, u8 reply_bytes)
	// CSPIMaster::WriteRead can perform both writes and reads in a single call.
	// For instance, to write one byte and read two more, you pass in one buffer of
	// len 3 for both input and output, where the first byte is the command
	// and the other two zeros will be replaced with the read data
	// THERE IS A DUMMY CYCLE !!! see ILI9488 datasheet Page 47
	// For single byte reads there is no dummy clock cycle.
	// For multiple byte reads there is a dummy clock cycle
	// right after the command byte.
	// To emulate the dummy clock cycle on reads of more than 1 byte
	// we read an extra byte and do the shifts ourselves.
{
	u8 buf[20];
	memset(buf,0,20);
	buf[0] = command;

	u8 extra_byte = reply_bytes > 1 ? 1 : 0;

    m_pSPI->SetClock(SPI_READ_FREQ);
	m_pinCD.Write(0);
	CTimer::Get()->usDelay(1);
	m_pSPI->WriteRead(0,buf,buf,reply_bytes + 1 + extra_byte);
	m_pinCD.Write(1);

	// for the bytes AFTER the command byte (+1)
	// shift the higher order bit of the NEXT (+2)
	// byte into the low order bit ..

	if (reply_bytes > 1)
	{
		for (int i=0; i<reply_bytes; i++)
		{
			u16 w = buf[i+1];
			w <<= 1;
			w |= (buf[i+2] & 0x80) ? 1 : 0;
			u8 b = w & 0xff;
			buf[i+1] = b;
		}
	}

	CString msg;
	msg.Format("%s(%d)",what,reply_bytes);
	for (int i=0; i<reply_bytes; i++)
	{
		CString byte_str;
		byte_str.Format(" 0x%02x",buf[i+1]);
		msg.Append(byte_str);
	}

	LOG(msg,0);

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

    LOG("ILI9488::initialize() TRIGGER_PIN=%d PAULS_INIT=%d WITH_BOGUS=%d",
		WITH_TRIGGER_PIN, PAULS_INIT, WITH_BOGUS);

	// We are not using the reset pin
    // m_pinRESET.Write(1);
    // CTimer::Get()->usDelay(1000);
    // m_pinRESET.Write(0);
    // CTimer::Get()->usDelay(300);
    // m_pinRESET.Write(1);
    // CTimer::Get()->usDelay(1000);
    // m_pinRESET.Write(1);


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

	setRotation(0);

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
        LOG("OUTPUT_TEST_PATTERN",0,0);
        fillRect(0,0,GetWidth()-1,GetHeight()-1,C_BLACK);
        fillRect(GetWidth()/3,GetHeight()/3,2*GetWidth()/3-1,2*GetHeight()/3-1,C_CYAN);
        fillRect(0,0,20,20,C_RED);
        fillRect(GetWidth()-1-30,0,GetWidth()-1,30,C_GREEN);
        fillRect(0,GetHeight()-1-40,40,GetHeight()-1,C_BLUE);
        fillRect(GetWidth()-1-50,GetHeight()-1-50,GetWidth()-1,GetHeight()-1,C_WHITE);

		printString(30,30,"This is a TEST",C_WHITE);
    #endif

	return true;
}


void ILI9488::printString(s16 x, s16 y, const char *str, u16 color)
{
	s16 char_width = m_CharGen.GetCharWidth();
	while (*str)
	{
		CScreenDeviceBase::DisplayChar(*str++, x, y, color);
		x += char_width;
	}
}


void ILI9488::color565ToBuf(u16 color, u8 *buf)
	// buffer is always 3 bytes JIC we are in 18 bit mode
{
	#if USE_16_BIT_COLORS
		buf[0] = color >> 8;
		buf[1] = color & 0xff;
	#else
		buf[0] = (color >> 11) & 0x1f;
		buf[0] <<= 3;
		buf[1] = (color >> 5) & 0x3f;
		buf[1] <<= 2;
		buf[2] = color & 0x1f;
		buf[2] <<= 3;
	#endif
}



//--------------------------------------------------------
// API
//---------------------------------------------------------

// virtual
unsigned ILI9488::GetWidth() const
{
	return m_rotation & 1 ? HEIGHT : WIDTH;
}

// virtual
unsigned ILI9488::GetHeight() const
{
	return m_rotation & 1 ? WIDTH : HEIGHT;
}


void ILI9488::setRotation(u8 rotation)
{
	rotation = rotation % 4;
	m_rotation = rotation;

	u8 buf[1];
	buf[0] = 0x48; 	// column_order | bgr
	switch (m_rotation)
	{
		case 1:		// 90:
			buf[0] = 0x28;	// rc exchange | bgr
			break;
		case 2: 	// 180:
			buf[0] = 0x88;  // row_order | bgr
			break;
		case 3:		// 270:
			buf[0] = 0xE8;	// rc exchange | row_order | bgr
			break;
	}

	writeCommand(0x36,buf,1);	// MADCTL
	CTimer::Get()->usDelay(5);
}


// private
void ILI9488::setWindow(int xs, int ys, int xe, int ye)
{
    u8 buf[4];
    buf[0] = xs >> 8;
    buf[1] = xs & 0xFF;
    buf[2] = xe >> 8;
    buf[3] = xe & 0xFF;
    writeCommand(0x2A,buf,4);	// SET_COLUMN_ADDRESS

    buf[0] = ys >> 8;
    buf[1] = ys & 0xFF;
    buf[2] = ye >> 8;
    buf[3] = ye & 0xFF;
    writeCommand(0x2B,buf,4);	// SET_PAGE_ADDRESS

    writeCommand(0x2C,0,0); 	// WRITE_MEMORY_START
}


void ILI9488::fillRect(int xs, int ys, int xe, int ye, u16 color)
{
	// LOG("fillRect(%d,%d,%d,%d,0x%04x)",xs,ys,xe,ye,color);

	u8 buf[PIXEL_BYTES];
	color565ToBuf(color,buf);
    u32 pixels = (xe-xs+1) * (ye-ys+1);

	setWindow(xs,ys,xe,ye);
    while (pixels--)
    {
		m_pSPI->SetClock(SPI_WRITE_FREQ);
        m_pSPI->WriteRead(0,buf,0,PIXEL_BYTES);
    }
}


// virtual
void ILI9488::SetPixel(unsigned x, unsigned y, u16 color)
{
	if (x>GetWidth()) return;
	if (y>GetHeight()) return;

	u8 buf[PIXEL_BYTES];
	color565ToBuf(color,buf);

    setWindow(x,y,x,y);
    m_pSPI->SetClock(SPI_WRITE_FREQ);
    m_pSPI->Write(0,buf,PIXEL_BYTES);
}


//------------------------------------------
// optimized callback (ugui) routines
//------------------------------------------

// static
s8 ILI9488::staticFillFrame(void *pThis, s16 x1, s16 y1, s16 x2, s16 y2, u16 color )
{
	assert(pThis);
	((ILI9488*)pThis)->fillRect(x1,y1,x2,y2,color);
	return 0;
}


// static
void *ILI9488::staticFillArea(void *pThis, s16 x1, s16 y1, s16 x2, s16 y2)
	// FillArea sets up the window and returns a pointer to pushPixel
	// which is then called for each pixel to write it.
{
	assert(pThis);
	((ILI9488*)pThis)->setWindow(x1,y1,x2,y2);
	return (void *) &staticPushPixel;
}


// static
void ILI9488::staticPushPixel(void *pThis, u16 color)
{
	assert(pThis);
	u8 buf[PIXEL_BYTES];
	color565ToBuf(color,buf);
	((ILI9488*)pThis)->m_pSPI->SetClock(SPI_WRITE_FREQ);
	((ILI9488*)pThis)->m_pSPI->Write(0,buf,PIXEL_BYTES);
}
