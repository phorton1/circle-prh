//-----------------------------------------------------
// ili9586.cpp
//-----------------------------------------------------
// Device driver for 320x480 ili9486 based SPI TFT Display.
// Tested against Blue 3.5" rPi Hat with and without XPT2046

#include "ili9486.h"
#include <circle/logger.h>
#include <circle/gpiomanager.h>
#include <circle/util.h>
#include <utils/myUtils.h>


#define OUTPUT_TEST_PATTERN	1
	// on by default, the screen displays a distinct pattern
	// of squares to verify basic functionality at boot ...

#define log_name "ili9846s"
#define SPI_FREQ  26000000		// 26000000    // 500000 1000000 2000000 5000000 10000000 20000000 50000000
    // it appears to work up to about 40Mhz
    // 26Mhz is an arbitrary choice

#define WIDTH		320
#define HEIGHT		480

// some colors for testing
// definitions from ugui 565 colors

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
// prh 2024-04-06 - Grumble.  The ""datasheet" for the "Blue rPi 3.5" 320x480 ILI9486
// SPI" XPT2046 Touch Screen is useless.  It does not agree with the Wiki Page, and
// I believe is inherently incorrect.  These values are from the Wiki Page, and agree
// with my (working, tested) implementation.
//
// 11 	gpio17		TP_IRQ 	        Touch panel interrupt, pulled low during touch
// 18 	gpio24		LCD_RS 	        LCD instruction control, Instruction/Data register selection
//									called 'CD' in my code
// 19 	spi_mosi	LCD_SI/TP_SI 	SPI data input of both LCD & touch panel
// 21 	spi_miso	TP_S0 	        SPI data output of touch panel
// 22 	gpio25		RST 	        Reset
// 23 	spi_sclk	LCD_SCK/TP_SCK 	SPI clock for both LCD & touch panel
// 24 	gpio8		LCD_CS 	        LCD chip select (active low)
// 26 	gpio27		TP_CS 	        Touch panel chip select (active low)

// #define PIN_CS_TOUCH    7
// #define PIN_CS_LCD      8
// #define PIN_MISO        9
// #define PIN_MOSI        10
// #define PIN_SCLK        11
// #define PIN_TP_IRQ      17


#define PIN_CD          24
#define PIN_RESET       25
	// prh 2024-04-06 - pin25 conflicts with my current SYSTEM_RPI_READY pin
#define PIN_DEBUG       6
	// prh 2024-04-06 - this debug pin is virtually useless, it turns on
	// if the Initialize() method is called.
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
    m_pSPI(pSPI),
    m_pinCD(PIN_CD,GPIOModeOutput),
    m_pinRESET(PIN_RESET,GPIOModeOutput)
	#if WITH_TRIGGER_PIN
		,m_trigger_pin(WITH_TRIGGER_PIN,GPIOModeOutput)
	#endif
{
    LOG("ctor",0,0);
	m_rotation = 0;
	#if WITH_TRIGGER_PIN
		m_trigger_pin.Write(1);
	#endif
}


// virtual
void ILI9846::InitializeUI(void *pUI, DriverRegisterFxn registerFxn)
{
	LOG("InitializeUI()",0);
	registerFxn(pUI, this, SCREEN_OPT_FILL_FRAME, (void *) staticFillFrame );
	registerFxn(pUI, this, SCREEN_OPT_FILL_AREA, (void *) staticFillArea );
}


void ILI9846::write(u8 *data, u16 len)
{
    CTimer::Get()->usDelay(1);
	while (len--)
	{
		m_pSPI->SetClock(SPI_FREQ);
		m_pSPI->Write(0,data++,1);
		CTimer::Get()->usDelay(1);
	}
}


void ILI9846::writeCommand(u8 command, u8 *data, u16 len)
{
    m_pinCD.Write(0);
    write(&command,1);
    m_pinCD.Write(1);
    write(data,len);
}


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
		// prh - right now it's upside down on my desk :-)

    #if OUTPUT_TEST_PATTERN

        LOG("OUTPUT_TEST_PATTERN",0,0);

		#if WITH_TRIGGER_PIN
			m_trigger_pin.Write(0);
			CTimer::Get()->usDelay(5);
		#endif

        fillRect(0,0,GetWidth()-1,GetHeight()-1,C_BLACK);
        fillRect(GetWidth()/3,GetHeight()/3,2*GetWidth()/3-1,2*GetHeight()/3-1,C_CYAN);

        fillRect(0,0,20,20,C_RED);
        fillRect(GetWidth()-1-30,0,GetWidth()-1,30,C_GREEN);
        fillRect(0,GetHeight()-1-40,40,GetHeight()-1,C_BLUE);
        fillRect(GetWidth()-1-50,GetHeight()-1-50,GetWidth()-1,GetHeight()-1,C_WHITE);

    #endif

	return true;
}



// virtual
unsigned ILI9846::GetWidth() const
{
	return m_rotation & 1 ? HEIGHT : WIDTH;
}

// virtual
unsigned ILI9846::GetHeight() const
{
	return m_rotation & 1 ? WIDTH : HEIGHT;
}


void ILI9846::setRotation(u8 rotation)
{
	rotation = rotation % 4;
	m_rotation = rotation;
	LOG("setRotation(%d)",m_rotation);

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

	writeCommand(0x36,buf,1);      // MADCTL
}


void ILI9846::setWindow(int xs, int ys, int xe, int ye)
{
    u8 buf[4];
    buf[0] = xs >> 8;
    buf[1] = xs & 0xFF;
    buf[2] = xe >> 8;
    buf[3] = xe & 0xFF;
    writeCommand(0x2A,buf,4);       // MIPI_DCS_SET_COLUMN_ADDRESS

    buf[0] = ys >> 8;
    buf[1] = ys & 0xFF;
    buf[2] = ye >> 8;
    buf[3] = ye & 0xFF;
    writeCommand(0x2B,buf,4);       // MIPI_DCS_SET_PAGE_ADDRESS

    writeCommand(0x2C,0,0);      // MIPI_DCS_WRITE_MEMORY_START
}



//--------------------------------------------
// display routines
//--------------------------------------------


void ILI9846::fillRect(int xs, int ys, int xe, int ye, u16 color)
{
    setWindow(xs,ys,xe,ye);
    u32 pixels = (xe-xs+1) * (ye-ys+1);

    u8 buf[2];
    buf[0] = color >> 8;
    buf[1] = color & 0xff;

    while (pixels--)
    {
		m_pSPI->SetClock(SPI_FREQ);
        m_pSPI->Write(0,buf,2);
    }
}


// virtual
void ILI9846::SetPixel(unsigned x, unsigned y, u16 color)
{
	if (x>GetWidth()) return;
	if (y>GetHeight()) return;

    setWindow(x,y,x,y);
    u8 buf[2];
    buf[0] = color >> 8;
    buf[1] = color & 0xff;
	m_pSPI->SetClock(SPI_FREQ);
    m_pSPI->Write(0,buf,2);
}


//------------------------------------------
// optimized callback (ugui) routines
//------------------------------------------

// static
s8 ILI9846::staticFillFrame(void *pThis, s16 x1, s16 y1, s16 x2, s16 y2, u16 color )
{
	assert(pThis);
	((ILI9846*)pThis)->fillRect(x1,y1,x2,y2,color);
	return 0;
}



// static
void *ILI9846::staticFillArea(void *pThis, s16 x1, s16 y1, s16 x2, s16 y2)
	// FillArea sets up the window and returns a pointer to pushPixel
	// which is then called for each pixel to write it.
{
	assert(pThis);
	((ILI9846*)pThis)->setWindow(x1,y1,x2,y2);
	return (void *) &staticPushPixel;
}


// static
void ILI9846::staticPushPixel(void *pThis, u16 color)
{
	assert(pThis);
	u8 buf[2];
	buf[0] = color >> 8;
	buf[1] = color & 0xff;
	((ILI9846*)pThis)->m_pSPI->SetClock(SPI_FREQ);
	((ILI9846*)pThis)->m_pSPI->Write(0,buf,2);
}
