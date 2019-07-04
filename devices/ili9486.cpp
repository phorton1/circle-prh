// xpt2046

#include "ili9486.h"
#include <circle/logger.h>
#include <circle/gpiomanager.h>
#include <circle/util.h>


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
// 11 	gpio17		TP_IRQ 	        Touch panel interrupt, pulled low during touch
// 18 	gpio24		LCD_RS 	        LCD instruction control, Instruction/Data register selection
// 19 	spmos1		LCD_SI/TP_SI 	SPI data input of both LCD & touch panel
// 21 	spmos0		TP_S0 	        SPI data output of touch panel
// 22 	gpio25		RST 	        Reset
// 23 	spisclk		LCD_SCK/TP_SCK 	SPI clock for both LCD & touch panel
// 24 	spice0		LCD_CS 	        LCD chip select (active low)
// 26 	spice1		TP_CS 	        Touch panel chip select (active low)


// #define PIN_CS_TOUCH    7
// #define PIN_CS_LCD      8
// #define PIN_MISO        9
// #define PIN_MOSI        10
// #define PIN_SCLK        11
// #define PIN_TP_IRQ      17
#define PIN_CD          24
#define PIN_RESET       25

#define PIN_DEBUG       6


#define DLY  255


u8 init_sequence[] =
{
//	2, 0xb0, 0x0,	// Interface Mode Control
//	1, 0x11,		// Sleep OUT
//	DELAY, 150,
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




ILI9846 *ILI9846::s_pThis = 0;




ILI9846::~ILI9846()
{
}

 
ILI9846::ILI9846(CSPIMaster *pSPI) :
    m_pSPI(pSPI),
    m_pinCD(PIN_CD,GPIOModeOutput),
    m_pinRESET(PIN_RESET,GPIOModeOutput)
	#ifdef ILI9846_WITH_DEBUG_PIN
		,m_pinDebug(PIN_DEBUG,GPIOModeOutput)
	#endif
{
	m_rotation = 0;
	s_pThis = this;
    LOG("ctor",0,0);
}
        

// virtual
void ILI9846::provideUGOptimizations()
{
	LOG("provideUGOptimizations()",0);
	UG_DriverRegister( DRIVER_DRAW_LINE, (void *) drawLine );
	UG_DriverRegister( DRIVER_FILL_FRAME, (void *) fillFrame );
	UG_DriverRegister( DRIVER_FILL_AREA, (void *) fillArea );    
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

    #if ILI9846_WITH_DEBUG_PIN
        m_pinDebug.Write(1);
        CTimer::Get()->usDelay(50);
    #endif
    
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
	
    #if 1
        LOG("test",0,0);
		#if ILI9846_WITH_DEBUG_PIN
			m_pinDebug.Write(1);
		#endif
		
        fillRect(0,0,getWidth()-1,getHeight()-1,C_BLACK);
        fillRect(getWidth()/3,getHeight()/3,2*getWidth()/3-1,2*getHeight()/3-1,C_CYAN);
		
        fillRect(0,0,20,20,C_RED);
        fillRect(getWidth()-1-30,0,getWidth()-1,30,C_GREEN);
        fillRect(0,getHeight()-1-40,40,getHeight()-1,C_BLUE);
        fillRect(getWidth()-1-50,getHeight()-1-50,getWidth()-1,getHeight()-1,C_WHITE);
    #endif
	
	return true;
}



unsigned ILI9846::getWidth() const
{
	return m_rotation & 1 ? HEIGHT : WIDTH;
}

unsigned ILI9846::getHeight() const
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


void ILI9846::setPixel(UG_S16 x, UG_S16 y, UG_COLOR color)
{
	if (x<0 || x>WIDTH) return;
	if (y<0 || y>HEIGHT) return;
	
    setWindow(x,y,x,y);
    u8 buf[2];
    buf[0] = color >> 8;
    buf[1] = color & 0xff;
	m_pSPI->SetClock(SPI_FREQ);
    m_pSPI->Write(0,buf,2);
}


//-------------------------------
// optimized UGUI routines
//-------------------------------

// static
UG_RESULT ILI9846::fillFrame( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_COLOR c )
{
	if (s_pThis)
	{
		s_pThis->fillRect(x1,y1,x2,y2,c);
	}
	return UG_RESULT_OK;
}


#define swap(i,j)  { UG_S16 tmp; tmp=i; i=j; j=tmp; }

// static
UG_RESULT ILI9846::drawLine( UG_S16 x0, UG_S16 y0, UG_S16 x1, UG_S16 y1, UG_COLOR c )
{
	if (s_pThis)
	{
		if ((y0 < 0 && y1 <0) || (y0 > HEIGHT && y1 > HEIGHT))
			return UG_RESULT_OK;
		if ((x0 < 0 && x1 <0) || (x0 > WIDTH && x1 > WIDTH))
			return UG_RESULT_OK;
		if (x0 < 0) x0 = 0;
		if (x1 < 0) x1 = 0;
		if (y0 < 0) y0 = 0;
		if (y1 < 0) y1 = 0;
		if (x0 > x1)
		{
			swap(x0,x1);
		}
		if (y0 > y1)
		{
			swap(y0,y1);
		}
		bool steep = (y1 - y0) > (x1 - x0);
		
		if (steep)
		{
			swap(x0, y0);
			swap(x1, y1);
		}
		if (x0 > x1) {
			swap(x0, x1);
			swap(y0, y1);
		}

		float distx = (x1-x0+1);
		float disty = (y1-y0+1);
		float dy = disty/distx;
		
		for (u16 x=x0; x<=x1; x++)
		{
			float ty = (x-x0) * dy;
			u16 y = ty;
			s_pThis->setPixel(x,y,c);
		}
	}
	return UG_RESULT_OK;
}



// static
void *ILI9846::fillArea( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2)
	// FillArea sets up the window and returns a pointer to pushPixel
	// which is then called for each pixel to write it.
{
	if (s_pThis)
	{
		s_pThis->setWindow(x1,y1,x2,y2);
	}
	return (void *) &pushPixel;
}


// static
void ILI9846::pushPixel(UG_COLOR c)
{
	if (s_pThis)
	{
		u8 buf[2];
		buf[0] = c >> 8;
		buf[1] = c & 0xff;
		s_pThis->m_pSPI->SetClock(SPI_FREQ);
		s_pThis->m_pSPI->Write(0,buf,2);
	}
}




