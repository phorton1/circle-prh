#ifndef __ilibase_h__
#define __ilibase_h__

#include <circle/gpiopin.h>
#include <circle/spimaster.h>
#include <circle/screen.h>


#define WITH_TRIGGER_PIN	0	// 21
	// An explicit trigger pin for use with logic analyzer
	// to capture specific moments.

// a few RGB565 colors for testing

#define RGB565_BLACK     0x0000
#define RGB565_BLUE      0x001F
#define RGB565_GREEN     0x07E0
#define RGB565_RED       0xF800
#define RGB565_YELLOW    0xFFE0
#define RGB565_CYAN      0x07FF
#define RGB565_WHITE     0xFFFF



class ILIBASE : public CScreenDeviceBase
	// Base class of ILE TFT devices that take RGB565 colors.
	// Provides common methods and members
{
public:

    ILIBASE(
		u16 fixed_width,
		u16 fixed_height,
		u8 pixel_bytes,			// 2 for 9486, 3 for 9488
		CSPIMaster *pSPI,
		u32 spi_write_freq,
		u32 spi_read_freq );
    ~ILIBASE();

    // CScreenDeviceBase overrides

    virtual boolean Initialize(void) = 0;
		// must be provided by derived class
	/* virtual */ virtual void InitializeUI(void *pUI, DriverRegisterFxn registerFxn) override;
    /* virtual */ virtual unsigned GetWidth(void) const override;
    /* virtual */ virtual unsigned GetHeight(void) const override;
    /* virtual */ virtual void SetPixel(unsigned x, unsigned y, u16 color) override;
    /* virtual */ virtual u16 GetPixel(unsigned x, unsigned y)  override { return 0; }

    u8 getRotation() { return m_rotation; }
    void setRotation(u8 rotation);
    void fillRect(int xs, int ys, int xe, int ye, u16 color);

    // static methods that can be registered with ugui
    // FillArea sets up the window and returns a pointer to pushPixel
    // which is then called for each pixel to write it.

    static s8 staticFillFrame(void *pThis, s16 x1, s16 y1, s16 x2, s16 y2, u16 color);
    static void *staticFillArea(void *pThis, s16 x1, s16 y1, s16 x2, s16 y2);
    static void staticPushPixel(void *pThis, u16 color);

	void distinctivePattern();
		// outputs 20 pixel red square in upper left corner
		// 30 green in upper right, blue 40 lower left, and
		// white 50 pin bottom right


	u16	charWidth();
	u16	charHeight();
	void printChar(u16 x, u16 y, char c, u16 color, u8 size);
	void printString(u16 x, u16 y, const char *str, u16 color, u8 size /*=1*/);
		// rudimentary character output


protected:

	u8 			m_rotation;
	u16			m_fixed_width;
	u16			m_fixed_height;
	u8			m_pixel_bytes;
    CSPIMaster *m_pSPI;
	u32 		m_write_freq;
	u32 		m_read_freq;
	CGPIOPin    m_pinCD;

	#if WITH_TRIGGER_PIN
		CGPIOPin    m_trigger_pin;
	#endif

 	CCharGenerator m_CharGen;

    void write(u8 *data, u16 len);
    void writeCommand(u8 command, u8 *data, u16 len);
    void setWindow(int xs, int ys, int xe, int ye);

	void dbgRead(const char *what, u8 command, u8 num_reply_bytes);

	virtual void color565ToBuf(u16 color, u8 *buf) = 0;
		// must be provided by derived class
		// to provide m_pixel_bytes in buf

};

#endif      // !__ilibase_h__
