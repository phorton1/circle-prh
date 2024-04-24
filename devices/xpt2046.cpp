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


//------------------------------------------------
// defines for calibration
//------------------------------------------------

#define CALIB_DOWN  0
#define CALIB_MOVE  1
#define CALIB_UP    2

#define BUTTON_DELAY  15
#define CANCEL_DELAY  25

#define BUTTON_WIDTH 	80
#define BUTTON_HEIGHT 	40


//-------------------------------------------------
// implementation
//-------------------------------------------------



XPT2046::~XPT2046()
{
}


XPT2046::XPT2046(CSPIMaster *pSPI, ILIBASE *pTFT) :
    m_pSPI(pSPI),
	m_pTFT(pTFT),
	m_pFileSystem(0)
{
	m_rotation = pTFT->getRotation();
    m_width = pTFT->GetWidth();
    m_height = pTFT->GetHeight();
    LOG("XPT046(%d,%d,%d)",m_rotation,m_width,m_height);

    m_lastx = 0;
    m_lasty = 0;
    m_lastz = 0;

	// arbitrary starting calibration values from
	// emprical testing on a orange ILI9488 device

	m_calibration_phase = 0;
	m_min_x = 241;
	m_max_x = 3830;
	m_min_y = 174;
	m_max_y = 3888;

}



void XPT2046::setRotation(u8 rotation)
{
	m_rotation = rotation;
	LOG("setRotation(%d)",m_rotation);
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

	// GRRR - there is always a leading zero bit returned
	// therefore, for 12 bits we right shift THREE bits.
	// The doc is terribly unclear about what the pattern
	// is in 8 bit mode (leading or trailing?)

	u16 retval = buf[1];
	retval <<= 8;
	retval |= buf[2];
	retval >>= 3;

	//	if (reg & XPT_8BIT_MODE)
	//	{
	//		retval &= 0xff;
	//	}
	//	else	// 12 bit mode
	//	{
	//		retval &= 0xfff;
	//	}

	return retval;
}



//---------------------------------------------------------
// IMPLEMENTATION
//---------------------------------------------------------


#define swap(i,j)  { s16 tmp; tmp=i; i=j; j=tmp; }


void XPT2046::Update()
{
	// Cancel or advance the calibration phase based on timer

	if (m_calibration_phase)
	{
		u32 elapsed = CTimer::Get()->GetTime() - m_calibration_time;
		u32 remain  = elapsed > BUTTON_DELAY ?
			CANCEL_DELAY - elapsed :
			BUTTON_DELAY - elapsed;
		if (remain != m_display_time)
		{
			m_display_time = remain;

			// fill the background ....

			#define RECT_CHARS 6

			s16 width = m_pTFT->GetWidth();
			s16 char_width = m_pTFT->charWidth() * 2;
			s16 char_height = m_pTFT->charHeight() * 2;
			s16 start_x = (width - RECT_CHARS*char_width) / 2;
			s16 ex = start_x + RECT_CHARS * char_width - 1;
			s16 ey = 20 + char_height - 1;
			m_pTFT->fillRect(start_x,20,ex,ey,RGB565_BLACK);

			// output a timer string

			CString msg;
			msg.Format ("%d",m_display_time);
			printCentered(2,20,msg);
		}

		// advance or cancel m_calibration_phase

		if (elapsed > CANCEL_DELAY)
		{
			endCalibration(0);
		}
		else if (m_calibration_phase == 1 && elapsed > BUTTON_DELAY)
		{
			m_calibration_phase++;
			showButton(1,0);
			showButton(2,0);
		}
	}

	// Another thing the doc just completely fails to describe
	// is what are z1 and z2 and how to use them.  From the
	// TFT_ESPI implementation, I gleaned this weird bit of
	// code that subtracts z2 from z1 and biases it by 0xfff,
	// then uses a threshold (350 typically).  In the end
	// I went with my MUCH simpler code.

	#if 0
		#define Z_THRESH  128
		s16 z1 = transfer16(0xB0);
		s16 z2 = transfer16(0xC0);
		s16 z3 = 0xfff + z1 - z2;
		s16 z = z3 > Z_THRESH ? 1 : 0;
	#else
		// read z1 three times and only set z to 1 if all three have values
		s16 z1 = transfer16(XPT_READ_Z1);	// 0xB9
		s16 z2 = transfer16(XPT_READ_Z1);	// 0xB9
		s16 z3 = transfer16(XPT_READ_Z1);	// 0xB9
		s16 z = z1 && z2 && z3 ? 1 : 0;
	#endif

	// x and y are only valid if z, so
	// if z, get, then scale and rotate x and y

	if (z)
	{
		s16 x  = transfer16(XPT_READ_X);	// 0xD1
		s16 y  = transfer16(XPT_READ_Y);	// 0x91

		#if DEBUG_TOUCH
			LOG("z(%d) z1(%d) z2(%d) z3(%d) x(%d) y(%d)",z,z1,z2,z3,x,y);
		#endif

		if (m_calibration_phase)
		{
			#define CALIB_FUDGE_Y  30
			#define CALIB_FUDGE_X  -10

			if (x+CALIB_FUDGE_X > m_max_x) m_max_x = x+CALIB_FUDGE_X;
			if (x < m_min_x) m_min_x = x;
			if (y > m_max_y) m_max_y = y;
			if (y+CALIB_FUDGE_Y < m_min_y) m_min_y = y+CALIB_FUDGE_Y;

			LOG("CALIB(%d,%d,%d) min(%d,%d) max(%d,%d)",x,y,z,m_min_x,m_min_y,m_max_x,m_max_y);
		}

		// scale to screen size

		float fx = x - m_min_x;
		float fy = y - m_min_y;
		if (fx < 0) fx = 0;
		if (fy < 0) fy = 0;
		fx = fx /(m_max_x-m_min_x);
		fy = fy /(m_max_y-m_min_y);
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
		// we send all of em to calibMove if in a calibration

		if (!m_lastz)
		{
			if (m_calibration_phase)
			{
				calibMove(CALIB_DOWN,x,y);
			}
			else if (m_pEventHandler)
			{
				m_pEventHandler(m_pThat,TouchScreenEventFingerDown,0,x,y);
			}
		}
		else if (x != m_lastx || y != m_lasty)
		{
			if (m_calibration_phase)
			{
				calibMove(CALIB_MOVE,x,y);
			}
			#ifdef GENERATE_MOVE_EVENTS
				else if (m_pEventHandler)
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

	else if (m_lastz)
	{
		if (m_calibration_phase)
		{
			calibMove(CALIB_UP,m_lastx,m_lasty);
		}
		else if (m_pEventHandler)
		{
			m_pEventHandler(m_pThat,TouchScreenEventFingerUp,0,m_lastx,m_lasty);
		}
	}

	m_lastz = z;

}



//------------------------------------------------------------
// Calibration UI
//------------------------------------------------------------
// THERE IS A LIMITATION TO THE PRECISNESS OF TOUCHES
// BUTTONS SHOULD BE AT LEAST 30 x 30, preferably 40x40
// and the edges of the screen are questionable.
//
// Using the screen itself for calibration Cancel/OK buttons
// is weird, inasmuch as they could get a false hit on an OK
// button while moving the mouse around.
//
// Therefore there are two phases, and a timeout ...
// For 15 seconds there are no buttons.
// 		hopefully this is enough time to start getting a line
// Then for 10 seconds the buttons show.
// If the OK button is not pressed, then a cancel is performed


void XPT2046::startCalibration()
{
	LOG("startCalibration()",0);

	int width = m_pTFT->GetWidth();
	int height = m_pTFT->GetHeight();
	int char_height = m_pTFT->charHeight();

	m_pTFT->fillRect(0,0,width-1,height-1,RGB565_BLACK);

	int start_y = (height - 4 * char_height) / 2;

	start_y = printCentered(2, start_y,
		"CALIBRATE TOUCH");
	start_y = printCentered(1, start_y,
		"Slide pointer towards the edges until");
	start_y = printCentered(1, start_y,
		"you get a good set of lines");

	m_save_min_x = m_min_x;
	m_save_max_x = m_max_x;
	m_save_min_y = m_min_y;
	m_save_max_y = m_max_y;

	m_min_x = 30000;
	m_max_x = 0;
	m_min_y = 30000;
	m_max_y = 0;

	m_button_pressed = 0;
	m_calibration_phase = 1;
	m_calibration_time = CTimer::Get()->GetTime();

	LOG("startCalibration() returning",0);
}


void XPT2046::endCalibration(bool ok)
{
	LOG("endCalibration(%d)",ok);

	int width = m_pTFT->GetWidth();
	int height = m_pTFT->GetHeight();
	int char_height = m_pTFT->charHeight();
	int start_y = (height - 2 * char_height) / 2;

	m_pTFT->fillRect(0,0,width-1,height-1,RGB565_BLACK);

	if (ok)
	{
		printCentered(2, start_y,"CALIBRATION COMPLETE");
		writeCalibration();
	}
	else
	{
		printCentered(2, start_y,"CALIBRATION ABORTED");
		m_min_x = m_save_min_x;
		m_max_x = m_save_max_x;
		m_min_y = m_save_min_y;
		m_max_y = m_save_max_y;
	}

	CTimer::Get()->MsDelay(1000);
	m_calibration_phase = 0;
	LOG("endCalibration() returning",0);
}



int XPT2046::printCentered(int size, int start_y, const char *str)
{
	int width = m_pTFT->GetWidth();
	int len = strlen(str);
	int start_x = (width - (size * len * m_pTFT->charWidth())) / 2;
	m_pTFT->printString(start_x,start_y,str,RGB565_WHITE,RGB565_BLACK,size);
	return start_y + size * m_pTFT->charHeight();
}


void XPT2046::showButton(int num, bool down)
{
	int width = m_pTFT->GetWidth();
	int char_height = m_pTFT->charHeight();
	int char_width  = m_pTFT->charWidth();

	const char *msg = num==1 ? "Cancel" : "OK";
	u16 len = strlen(msg);

	u16 fg =
		down ? RGB565_WHITE : RGB565_BLACK;
	u16 bg =
		down ? RGB565_BLUE  :
		num == 1 ? RGB565_RED :
		RGB565_GREEN;
	u16 box_sx = num == 1 ? 0 : width - BUTTON_WIDTH;
	u16 box_ex = num == 1 ? BUTTON_WIDTH-1 : width-1;

	m_pTFT->fillRect(box_sx,0,box_ex,BUTTON_HEIGHT-1,bg);

	u16 text_y = (BUTTON_HEIGHT - char_height) / 2;
	u16 text_x = (BUTTON_WIDTH - char_width * len) / 2;

	m_pTFT->printString(box_sx + text_x,text_y,msg,fg,bg,1);
}


void XPT2046::calibMove(u16 state, u16 x, u16 y)
{
	int width = m_pTFT->GetWidth();

	int in = m_calibration_phase == 2 ?
		(y<50 && x<50) ? 1 :
		(y<50 && x>width-50) ? 2 : 0 : 0;

	// LOG("PHASE(%d) CALIB_%s(%d,%d)  in=%d  button=%d",
	// 	m_calibration_phase,
	// 	(state == 0 ? "DOWN" : state == 1 ? "MOVE" : "UP"),
	// 	x,y,in,m_button_pressed);

	// show the contact point as 3x3 yellow pixels

	if (!in && (state == CALIB_DOWN || state == CALIB_MOVE))
	{
		for (int i=-1; i<=1; i++)
		{
			for (int j=-1; j<=1; j++)
			{
				m_pTFT->SetPixel(x+i,y+j,RGB565_YELLOW);
			}
		}
	}

	if (m_calibration_phase == 2)
	{
		if (state == CALIB_UP)			// finished calibration
		{
			if (m_button_pressed && in == m_button_pressed)
				endCalibration(in == 2);
			m_button_pressed = 0;
		}
		else if (state == CALIB_DOWN)	// button pressed
		{
			m_button_pressed = in;
			if (m_button_pressed)
				showButton(m_button_pressed,1);
		}
		else if (m_button_pressed && in != m_button_pressed)	// moved out button invalid
		{
			showButton(m_button_pressed,0);
			m_button_pressed = 0;
		}
	}
}


//------------------------------------
// read and write calibration
//--------------------------------------

#define CALIB_FILENAME "SD:tft_calib.txt"


void XPT2046::writeCalibration()
{
	LOG("writeCalibration(%d,%d,%d,%d)",m_min_x,m_max_x,m_min_y,m_max_y);

	if (m_pFileSystem)
	{
		FIL file;
		if (f_open(&file, CALIB_FILENAME, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
		{
			CString s;
			s.Format("%d,%d,%d,%d\n",m_min_x,m_max_x,m_min_y,m_max_y);
			int len = s.GetLength();

			u32 wrote = 0;
			if (f_write(&file,(const char *)s,len,&wrote) == FR_OK)
			{
				LOG("wrote %s(%d,%d,%d,%d)",CALIB_FILENAME,m_min_x,m_max_x,m_min_y,m_max_y);
			}
			else
			{
				LOG_ERROR("Could not open %s",len,CALIB_FILENAME);
			}

			f_close(&file);
		}
		else
		{
			LOG_WARNING("Could open %s for writing",CALIB_FILENAME);
		}
	}
	else
	{
		LOG_WARNING("No file system for %s",CALIB_FILENAME);
	}
}



void XPT2046::Initialize(FATFS *pFileSystem)
{
	m_pFileSystem = pFileSystem;
	if (pFileSystem)
	{
		FIL file;
		FILINFO info;

		LOG("Initialize()",0);

		if (f_stat(CALIB_FILENAME, &info) == FR_OK)
		{
			u32 len = info.fsize;
			// LOG("%s len=%d",CALIB_FILENAME,len);
			if (len && len < 100)
			{
				if (f_open(&file, CALIB_FILENAME, FA_READ | FA_OPEN_EXISTING) == FR_OK)
				{
					u32 read = 0;
					char buf[128];
					memset(buf,0,128);
					// LOG("reading %d bytes from %s",len,CALIB_FILENAME);
					if (f_read(&file,buf,len,&read) == FR_OK)
					{
						// LOG("got %d bytes from %s",read,CALIB_FILENAME);

						int count = 0;
						u16 values[4];
						char *p = buf;

						while (count<4 && *p)
						{
							const char *sp = p;
							while (*p && *p >= '0' && *p <= '9')
							{
								p++;
							}
							if (*p)
							{
								*p++ = 0;

								// strtoul sheesh, no atoi, atol, strToInt, took an hour to figure out

								char *pEnd;
								unsigned long stupid;
								stupid = strtoul(sp, &pEnd, 10);
									// sheesh, need end ptr and base(10) param
								values[count++] = stupid;
							}
						}

						if (count == 4)
						{
							// xmin, xmax, ymin, ymax

							if (values[0] < 10   || values[0] > 2048 ||
								values[1] < 2048 || values[1] > 4095 ||
								values[2] < 10   || values[2] > 2048 ||
								values[3] < 2048 || values[3] > 4095 ||
								values[0] >= values[1] ||
								values[2] >= values[3] )
							{
								LOG_ERROR("bad values(%d,%d,%d,%d) in %s",values[0],values[1],values[2],values[3],CALIB_FILENAME);
							}
							else
							{
								LOG("got calibration values: %d,%d,%d,%d",values[0],values[1],values[2],values[3]);
								m_min_x = values[0];
								m_max_x = values[1];
								m_min_y = values[2];
								m_max_y = values[3];

								// SUCCESS!! return

								return;
							}
						}
						else
						{
							LOG_ERROR("wrong number of values(%d) in %s",count,CALIB_FILENAME);
						}
					}
					else
					{
						LOG_ERROR("read failed %s",len,CALIB_FILENAME);
					}

					f_close(&file);

				}
				else
				{
					LOG_ERROR("Could not open %s",len,CALIB_FILENAME);
				}
			}
			else
			{
				LOG_ERROR("Incorrect len(%d) for %s",len,CALIB_FILENAME);
			}
		}
		else
		{
			LOG_WARNING("Could not stat %s",CALIB_FILENAME);
		}
	}

	// otherwise, start a calibration at boot !!

	startCalibration();
}
