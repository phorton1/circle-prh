//---------------------------------------------------------
// ili_base.cpp
//---------------------------------------------------------

#include "ili_base.h"
#include <circle/logger.h>
// #include <circle/gpiomanager.h>
#include <circle/util.h>
#include <utils/myUtils.h>


#define log_name "ilibase"


// May want to turn these three off in production use

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


// some colors for testing
// definitions from ugui 565 colors




//---------------------------------------------------------------------------------
// pins
//---------------------------------------------------------------------------------
// For 3.5" Blue rPi ILI9486 and Orange 3.5" ILI9488 displays.
// These devices use standard rPi MOSI, MISO, and SCK, and SPICE0 pins.
//
// 11	spi_sclk		LCD_SCK/TP_SCK 	SPI clock for both LCD & touch panel
// 10	spi_mosi		LCD_SI/TP_SI 	SPI data input of both LCD & touch panel
// 9	spi_miso		TP_S0 	        SPI data output of touch panel
// 8	gpio8 spice0	LCD_CS 	        SPICE0 = LCD chip select (active low)
// 7	gpio7 spice1	TP_CS 	        SPICE1 = Touch panel chip select (active low)
//
// The Blue 9486 hat has it's RS/CD pin hardwired to GPIO24.
//		This conflicts with the hardwired OCTO_MULT3 pin,
//		which causes noise in the OCTO when we draw to the display.
//      While still in development I chose and tested GPIO4
//	    with the OCTO.
// The Blue 9486 hat also has hardwired GPIO25 RESET pin,
//		which conflicts with the RPI_READY pin I use in std_kernel.cpp
//		and which is built into the teensyEpression1-2 rPI connector.
//		I could change it to a different pin (6, 13, and 26 are
//		available in the looper as of this 2024-04-29 writing),
//		but since I am not going to use the Blue 9486 in the TE3,
//		I am leaving the RPI_READY gpio pin as GPIO25 in
//		std_kernel.cpp
// The use of GPIO4 for CD in my OCTO tests conflicts with the need to
// 	    generate I2S_MCLK on the rPi GPIO4 pin for the SGTL5000.
//      So my latest plan is to move the ILI CD pin to GPIO17,
//		on the same side as MOSI,MISCO, and SCLK, for TE3

// #define PIN_CS_TOUCH    7
// #define PIN_CS_LCD      8
// #define PIN_MISO        9
// #define PIN_MOSI        10
// #define PIN_SCLK        11

#ifdef USE_BLUE_9486	// not defined anywhere
	#define PIN_CD		24
		// original, as used on Blue rPi 3.5 ILI9486 SPI
		// shield (which was never built into a device)
		// and which conflicts with OCTO_MULT3
#elif defined(OCTO_TEST_SETUP)	// not defined anywhere
	// rpi with Octo setup which was never built into a device
	// Remember that the old Looper is using rPi 7" touch screen.
	// This pin was tested, works, and elimitates OCTO crackling
	// noises when drawing to the screen.
	#define PIN_CD		4
#elif 1
	// ACTUAL DEFINE - chosen for TE3 with SGTL5000
	#define PIN_CD		17
#endif



ILIBASE::~ILIBASE()
{
}



ILIBASE::ILIBASE(u16 fixed_width,
		u16 fixed_height,
		u8 pixel_bytes,
		ILISPI_CLASS *pSPI,
		u32 spi_write_freq,
		u32 spi_read_freq ) :
	m_fixed_width(fixed_width),
	m_fixed_height(fixed_height),
	m_pixel_bytes(pixel_bytes),
    m_pSPI(pSPI),
	m_write_freq(spi_write_freq),
	m_read_freq(spi_read_freq),
    m_pinCD(PIN_CD,GPIOModeOutput)
	#if WITH_TRIGGER_PIN
		,m_trigger_pin(WITH_TRIGGER_PIN,GPIOModeOutput)
	#endif
{
	m_rotation = 0;
	m_pinCD.Write(1);
	#if WITH_TRIGGER_PIN
		m_trigger_pin.Write(1);
	#endif
}


// virtual
void ILIBASE::InitializeUI(void *pUI, DriverRegisterFxn registerFxn)
{
	LOG("InitializeUI()",0);
	registerFxn(pUI, this, SCREEN_OPT_FILL_FRAME, (void *) staticFillFrame );
	registerFxn(pUI, this, SCREEN_OPT_FILL_AREA, (void *) staticFillArea );
}


void ILIBASE::write(u8 *data, u16 len)
{
    CTimer::Get()->usDelay(1);
	while (len--)
	{
		m_pSPI->SetClock(m_write_freq);
		m_pSPI->WriteRead(0,data++,0,1);
		CTimer::Get()->usDelay(1);
	}
}


void ILIBASE::writeCommand(u8 command, u8 *data, u16 len)
{
    m_pinCD.Write(0);
    write(&command,1);
    m_pinCD.Write(1);
    if (len)
		write(data,len);
}



void ILIBASE::dbgRead(const char *what, u8 command, u8 reply_bytes)
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

    m_pSPI->SetClock(m_read_freq);
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


//------------------------------
// output routines
//------------------------------

void ILIBASE::distinctivePattern()
{
	LOG("OUTPUT_TEST_PATTERN",0,0);
	fillRect(0,0,GetWidth()-1,GetHeight()-1,RGB565_BLACK);
	fillRect(GetWidth()/3,GetHeight()/3,2*GetWidth()/3-1,2*GetHeight()/3-1,RGB565_CYAN);
	fillRect(0,0,20,20,RGB565_RED);
	fillRect(GetWidth()-1-30,0,GetWidth()-1,30,RGB565_GREEN);
	fillRect(0,GetHeight()-1-40,40,GetHeight()-1,RGB565_BLUE);
	fillRect(GetWidth()-1-50,GetHeight()-1-50,GetWidth()-1,GetHeight()-1,RGB565_WHITE);

	printString(30,30,"This is a TEST",RGB565_YELLOW,RGB565_BLUE,2);
}


u16	ILIBASE::charWidth()
{
	return m_CharGen.GetCharWidth();
}


u16	ILIBASE::charHeight()
{
	return m_CharGen.GetCharHeight();
}


void ILIBASE::printChar(u16 x, u16 y, char c, u16 fg, u16 bg, u8 size)
{
	// LOG("    printChar(%d,%d,'%c',0x%04x,0x%04x,%d)",x,y,c,fg,bg,size);
	for (u16 yp=0; yp<charHeight(); yp++)
	{
		for (u16 xp=0; xp<charWidth(); xp++)
		{
			u16 col = m_CharGen.GetPixel(c, xp, yp) ? fg : bg;
			u16 sx = x + (xp * size);
			u16 sy = y + (yp * size);
			for (u16 i=0; i<size; i++)
			{
				SetPixel (sx+i,sy+i,col);
			}
		}
	}
}


void ILIBASE::printString(u16 x, u16 y, const char *str, u16 fg, u16 bg, u8 size /*=1*/)
{
	// LOG("printString(%d,%d,'%s',0x%04x,0x%04x, %d)",x,y,str,fg,bg,size);

	s16 char_width = charWidth();
	while (*str)
	{
		// CScreenDeviceBase::DisplayChar(*str++, x,y, color);
	    printChar(x, y, *str++, fg, bg, size);
	    x += char_width * size;
	}
}



//--------------------------------------------------------
// API
//---------------------------------------------------------

// virtual
unsigned ILIBASE::GetWidth() const
{
	return m_rotation & 1 ? m_fixed_height : m_fixed_width;
}

// virtual
unsigned ILIBASE::GetHeight() const
{
	return m_rotation & 1 ? m_fixed_width : m_fixed_height;
}


void ILIBASE::setRotation(u8 rotation)
{
	LOG("setRotation(%d)",rotation);
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
void ILIBASE::setWindow(int xs, int ys, int xe, int ye)
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


void ILIBASE::fillRect(int xs, int ys, int xe, int ye, u16 color)
{
	// LOG("fillRect(%d,%d,%d,%d,0x%04x)",xs,ys,xe,ye,color);

	u8 buf[m_pixel_bytes];
	color565ToBuf(color,buf);
    u32 pixels = (xe-xs+1) * (ye-ys+1);

	setWindow(xs,ys,xe,ye);
    while (pixels--)
    {
		m_pSPI->SetClock(m_write_freq);
        m_pSPI->WriteRead(0,buf,0,m_pixel_bytes);
    }
}


// virtual
void ILIBASE::SetPixel(unsigned x, unsigned y, u16 color)
{
	if (x>GetWidth()) return;
	if (y>GetHeight()) return;

	u8 buf[m_pixel_bytes];
	color565ToBuf(color,buf);

    setWindow(x,y,x,y);
    m_pSPI->SetClock(m_write_freq);
    m_pSPI->Write(0,buf,m_pixel_bytes);
}


//------------------------------------------
// optimized callback (ugui) routines
//------------------------------------------

// static
s8 ILIBASE::staticFillFrame(void *pThis, s16 x1, s16 y1, s16 x2, s16 y2, u16 color )
{
	assert(pThis);
	ILIBASE *self = (ILIBASE*) pThis;
	self->fillRect(x1,y1,x2,y2,color);
	return 0;
}


// static
void *ILIBASE::staticFillArea(void *pThis, s16 x1, s16 y1, s16 x2, s16 y2)
	// FillArea sets up the window and returns a pointer to pushPixel
	// which is then called for each pixel to write it.
{
	assert(pThis);
	ILIBASE *self = (ILIBASE*) pThis;
	self->setWindow(x1,y1,x2,y2);
	return (void *) &staticPushPixel;
}


// static
void ILIBASE::staticPushPixel(void *pThis, u16 color)
{
	assert(pThis);
	ILIBASE *self = (ILIBASE*) pThis;
	u8 buf[self->m_pixel_bytes];
	self->color565ToBuf(color,buf);
	self->m_pSPI->SetClock(self->m_write_freq);
	self->m_pSPI->Write(0,buf,self->m_pixel_bytes);
}
