//-------------------------------------------------
// xpt2046
//-------------------------------------------------
// Note that on Orange 3.5" 320x480 ili9488 screens, I had
// to redesign the teensyExpression2 lcd_connector and pull
// MISO first from the TOUCH_DO pin, and separate it from the the
// LCD_MISO pin by a 220 ohm resistor in order to be able to
// read from both the ili9488 and xpt2046 on the same SPI bus.
// Otherwise, with previous connector design, the xpt2046 MISO
// signal was pulled down to below 1 volt by the ili9488 MISO
// line and the xpt2046 did not work.
//
// MULTI-SAMPLING
//
// It is not clear to me that there is a benefit to multiple
// sampling.  On Arduino/ESP32's I have used various approaches
// including.
//
//		- multi-sampling and only accepting the results if
//        is unchanged
//	    - multi-sampling and throwing out the first sample
//	      and taking the average
//      - keeping a circular buffer of samples
//
// I am re-implementing this to initially do NO multi-sampling
// just to see how it works.
//
// CALIBRATION
//
// There are a couple of approaches we can take to calibrating
// the X-Y axis values. On my CNC machines with ili9431 3.25"
// xpt2046 devices, I have an explicit calibration method that
// allows the user to touch the four corners and then those values
// are saved to the SDCard and restored at startup.
//
// Another approach is to start with conservative calibration
// values and then to auto-adjust them during usage to any new
// MAX or MIN values that are found.  That is the algorithm
// currently used in this implementation.


#include "xpt2046.h"
#include <circle/logger.h>
#include <circle/util.h>
#include <utils/myUtils.h>

#define log_name "xpt2046"

#define DEBUG_TOUCH   0


#define GENERATE_MOVE_EVENTS
	// if not, only up and down events will be generated

#define SPI_FREQ  1000000
	// 1 Mhz works

// This XPT2046 implementation uses default rPi SCLK, MOSI, MISO pins,
// and only the default PIN_CS_TOUCH == SPICE1, which is gotten
// with the '1' param to circle's SPI WriteRead() methods.
// All the pins on a typical TFT are listed here for completeness.
//
// #define PIN_CS_TOUCH    	7	// SPICE1	pass 1 to SPI.Write/Read
// #define PIN_CS_LCD      	8	// SPICE0	pass 0 to SPI.Write; not used by XPT2046
// #define PIN_MISO        	9	// default rPi SPI MISO
// #define PIN_MOSI        	10	// default rPi SPI MOSI
// #define PIN_SCLK        	11	// default rPi SPI SCLK
// #define PIN_TP_IRQ      	17	// we are not using the touch IRQ
// #define PIN_CD          	24	// for the LCD only - not used by XPT2046
// #define PIN_RESET       	25	// not used on XPT2046, or ILI9488, for that matter


//------------------------------------------------------------------
// XPT2046 Command bytes
//------------------------------------------------------------------
// The doc is horrible and does not explain what is really going on.
// But, with SER_DFR(0) there are four possible commands in a table
// in the doc.   My previoius implementation used POWER(01) to leave
// the ADC powered up, but the doc says this is not necessary and
// my new implementation uses POWER(00).  Note that we use 8BIT
// mode for z measurement as per doc recommendation


#define XPT_START_BIT		0x80
#define XPT_8BIT_MODE       0x08
#define XPT_POWER_REF		0x02
#define XPT_POWER_ADC		0x01

		// 1xxx mspp
		// 1 			= start bit
		//  xxx 		= A2,A1,A0
		//      m 		= MODE(0) = 12 bit conversion mode; 1==8 bit mode
		//       s		= SER_DFR(0) = differential reference mode
		//        pp    = POWER(pp) = Reference and ADC is on/off.
#define XPT_READ_Y		(XPT_START_BIT | (0x1 << 4) | XPT_POWER_ADC)
		// 0x91 = 001 = +REF=YP -REF=YN +IN=XP m=Y_position  drivers=YP,YN
#define XPT_READ_Z1		(XPT_START_BIT | (0x3 << 4) | XPT_8BIT_MODE | XPT_POWER_ADC)
		// 0xB9 = 011 = +REF=YP -REF=XN +IN=XP m=Z1_position drivers=YP
#define XPT_READ_Z2		(XPT_START_BIT | (0x4 << 4) | XPT_8BIT_MODE | XPT_POWER_ADC)
		// 0xC9 = 100 = +REF=YP -REF=XN +IN=YN m=Z2_position drivers=YP
#define XPT_READ_X		(XPT_START_BIT | (0x5 << 4) | XPT_POWER_ADC)
		// 0xD1 = 101 = +REF=XP -REF=XN +IN=YP m=X_position  drivers=XP




//-------------------------------------------------
// implementation
//-------------------------------------------------



XPT2046::XPT2046(CSPIMaster *pSPI, u16 width, u16 height) :
    m_pSPI(pSPI)
{
    LOG("ctor",0,0);
	m_rotation = 3;
    m_width = width;
    m_height = height;
    m_lastx = 0;
    m_lasty = 0;
    m_lastz = 0;
}



XPT2046::~XPT2046()
{
}




u16 XPT2046::transfer16(u8 reg)
{
	u8 buf[3];
	buf[0] = reg;
	buf[1] = 0;
	buf[2] = 0;
	m_pSPI->SetClock(SPI_FREQ);
	int rslt = m_pSPI->WriteRead(1,buf,buf,3);
	CTimer::Get()->usDelay(5);
	assert(rslt == 3);
	// LOG("buf[0]=0x%02x buf[0]=0x%02x buf[0]=0x%02x",buf[0],buf[1],buf[2]);

	u16 retval = buf[1];
	retval <<= 8;
	retval |= buf[2];
	retval >>= 4;

	if (reg & XPT_8BIT_MODE)
	{
		retval &= 0xff;
	}
	else	// 12 bit mode
	{
		retval &= 0xfff;
	}
	return retval;
}



//---------------------------------------------------------
// IMPLEMENTATION
//---------------------------------------------------------

#define swap(i,j)  { s16 tmp; tmp=i; i=j; j=tmp; }


static u16 MIN_X = 30000;
static u16 MAX_X = 0;
static u16 MIN_Y = 30000;
static u16 MAX_Y = 0;


void XPT2046::Update()
{
	// read z1 three times and only set z to 1 if all three have values

	s16 z1 = transfer16(XPT_READ_Z1);	// 0xB9
	s16 z2 = transfer16(XPT_READ_Z1);	// 0xB9
	s16 z3 = transfer16(XPT_READ_Z1);	// 0xB9

	s16 z = z1 && z2 && z3 ? 1 : 0;

	// x and y are only valid if z, so
	// if z, get, then scale and rotate x and y

	if (z)
	{
		s16 x  = transfer16(XPT_READ_X);	// 0xD1
		s16 y  = transfer16(XPT_READ_Y);	// 0x91

		#if DEBUG_TOUCH
			LOG("z(%d) z1(%d) z2(%d) z3(%d) x(%d) y(%d)",z,z1,z2,z3,x,y);
		#endif

		// THERE IS A LIMITATION TO THE PRECISNESS OF TOUCHES
		// BUTTONS SHOULD BE AT LEAST 30 x 30, preferably 40x40
		// and the edges of the screen are questionable.
		//
		// Because of Z falling to zero at low values of Y,
		// we arbitrarily scale MIN_Y up a bit

		#define CALIB_FUDGE_Y  30
		#define CALIB_FUDGE_X  10

		if (x+CALIB_FUDGE_X > MAX_X) MAX_X = x+CALIB_FUDGE_X;
		if (x < MIN_X) MIN_X = x;
		if (y > MAX_Y) MAX_Y = y;
		if (y+CALIB_FUDGE_Y < MIN_Y) MIN_Y = y+CALIB_FUDGE_Y;

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

		// X and Y from the xpt2046 are ALWAYS in the un-rotated
		// state, 0<=X<=319 and 479<=Y<=0, with 0,0 at the bottom
		// left corner.

		switch (m_rotation)
		{
			case 0:
				y = m_height - y;
				break;

			case 1:
				x = m_width - x;
				y = m_height - y;
				swap(x,y);
				break;

			case 2:
				x = m_width - x;
				break;

			case 3:
				swap(x,y);
				break;
		}

		// if not previous Z it's a Down event,
		// or if GENERATE_MOVE_EVENTS and x or y changed, it's a Move event

		if (m_pEventHandler)
		{
			if (!m_lastz)
			{
				m_pEventHandler(m_pThat,TouchScreenEventFingerDown,0,x,y);
			}

			#ifdef GENERATE_MOVE_EVENTS
				else if (x != m_lastx || y != m_lasty)
				{
					m_pEventHandler(m_pThat,TouchScreenEventFingerMove,0,x,y);
				}
			#endif
		}

		// assign m_last values

		m_lastx = x;
		m_lasty = y;
	}

	// otherwise, if !z and m_lastz, its an Up event

	else if (m_pEventHandler && m_lastz)
	{
		m_pEventHandler(m_pThat,TouchScreenEventFingerUp,0,m_lastx,m_lasty);
	}

	m_lastz = z;

}
