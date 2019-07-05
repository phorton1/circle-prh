// xpt2046
// see ili9486.cpp for comments

#include "xpt2046.h"
#include <circle/logger.h>
#include <circle/util.h>


#define log_name "xpt2046"

#define SPI_FREQ  500000

#define PIN_CS_TOUCH    7
// #define PIN_CS_LCD      8
// #define PIN_MISO        9
// #define PIN_MOSI        10
// #define PIN_SCLK        11
// #define PIN_TP_IRQ      17
// #define PIN_CD          24
// #define PIN_RESET       25



XPT2046::XPT2046(CSPIMaster *pSPI) :
    m_pSPI(pSPI),
    m_pinCS_TOUCH(PIN_CS_TOUCH,GPIOModeOutput)
{
    LOG("ctor",0,0);
	m_rotation = 3;
    m_width = 320;
    m_height = 480;
    m_lastx = 0;
    m_lasty = 0;
    m_lastz = 0;
}


XPT2046::~XPT2046()
{
}


static u16 MIN_X = 300;
static u16 MAX_X = 3700;
static u16 MIN_Y = 240;
static u16 MAX_Y = 3700;



u16 XPT2046::transfer16(u8 reg)
{
	u8 in_buf[3];
	u8 out_buf[3];
	out_buf[0] = reg;
	out_buf[1] = 0;
	out_buf[2] = 0;
	m_pSPI->SetClock(SPI_FREQ);		
	int rslt = m_pSPI->WriteRead(1,out_buf,in_buf,3);
	assert(rslt == 3);
	u16 retval = (in_buf[1] << 8) | in_buf[2];
	return retval;
}


static s16 besttwoavg( s16 x , s16 y , s16 z )
{
	s16 da, db, dc;
	s16 reta = 0;
	if ( x > y ) da = x - y; else da = y - x;
	if ( x > z ) db = x - z; else db = z - x;
	if ( z > y ) dc = z - y; else dc = y - z;
	if ( da <= db && da <= dc ) reta = (x + y) >> 1;
	else if ( db <= da && db <= dc ) reta = (x + z) >> 1;
	else reta = (y + z) >> 1;
	return (reta);
}




#define swap(i,j)  { s16 tmp; tmp=i; i=j; j=tmp; }


void XPT2046::Update()
{
	u8 buf[1];
	u16 data[6];
	memset(data,0,12);
	m_pinCS_TOUCH.Write(0);

	buf[0] = 0xb1; 							// Z1
	m_pSPI->SetClock(SPI_FREQ);		
	m_pSPI->Write(1,buf,1);

	// my screen reliably sets z2 to zero if no presses
	// we only report mouse up and down events,
	// so we only check for changes in z state
	
	u16 z2 = transfer16(0x91) ? 1 : 0;		// X
	if (m_lastz == z2)
	{
		m_pinCS_TOUCH.Write(0);
		return;
	}

	if (z2)	// (z >= Z_THRESHOLD)
	{
		transfer16(0x91);  						 // dummy X measur
		data[0] = transfer16(0xD1 /* Y */) >> 3;
		data[1] = transfer16(0x91 /* X */) >> 3; // make 3 x-y measurements
		data[2] = transfer16(0xD1 /* Y */) >> 3;
		data[3] = transfer16(0x91 /* X */) >> 3;
	}
	else
	{
		m_pinCS_TOUCH.Write(0);
		m_lastz = z2;
		
		// report mouse up event to client, with the previous x,y coordinates
		
		printf("touch up(%d,%d)\n",m_lastx,m_lasty);
		if (m_pEventHandler)
			m_pEventHandler(TouchScreenEventFingerUp,0,m_lastx,m_lasty);
		return;
	}
	
	// sending low bit 0 powers down and re-enables the interrupt
	// so we do this one outside of the if loop
	
	data[4] = transfer16(0xD0 /* Y */) >> 3;
	data[5] = transfer16(0) >> 3;
	m_pinCS_TOUCH.Write(1);

	s16 x = 0;
	s16 y = 0;
	// s16 rawx = 0;
	// s16 rawy = 0;
	if (z2)
	{
		// rawx =
		x = besttwoavg( data[0], data[2], data[4] );
		// rawy =
		y = besttwoavg( data[1], data[3], data[5] );
		
		// self adjusting scaling factors
		
		if (x > MAX_X) MAX_X = x;
		if (x < MIN_X) MIN_X = x;
		if (y > MAX_Y) MAX_Y = y;
		if (y < MIN_Y) MIN_Y = y;
		
		// scale to screen size
			
		float fx = x - MIN_X;
		float fy = y - MIN_Y;
		if (fx < 0) fx = 0;
		if (fy < 0) fy = 0;
		fx = fx /(MAX_X-MIN_X);
		fy = fy /(MAX_Y-MIN_Y);
		if (fx > 1.0) fx = 1.0;
		if (fy > 1.0) fy = 1.0;
		fx *= m_width;
		fy *= m_height;
		x = fx;
		y = fy;
		
		// rotate as needed
		
		switch (m_rotation)
		{
			case 0: 
				x = m_width - x;
				y = m_height - y;
				break;
			case 1: 
				y = m_height - y;
				swap(x,y);
				break;
			case 3:
				x = m_width - x;
				swap(x,y);
				break;
		}
	}

	#if 0
		static int count = 0;
		printf("%-10d z2 %-5d  x %-5d  y %-5d     rawx %-5d  rawy %-5d\n",
			count++,z2,x,y,rawx,rawy);
		// printf("%-10d z1 %04x  z2 %04x  data %04x %04x %04x %04x %04x %04x  x %04x  y %04x\n",
		//	count++,z1,z2,data[0],data[1],data[2],data[3],data[4],data[5],x,y);
	#endif
	
	// report mouse down event with appropriate coordinates
	
	m_lastz = z2;
	m_lastx = x;
	m_lasty = y;
	
	printf("touch down(%d,%d)\n",m_lastx,m_lasty);
	if (m_pEventHandler)
		m_pEventHandler(TouchScreenEventFingerDown,0,m_lastx,m_lasty);
	
}





