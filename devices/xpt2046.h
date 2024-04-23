#ifndef __xpt2046_h__
#define __xpt2046_h__


#include <circle/spimaster.h>
#include <circle/screen.h>
#include <circle/input/touchscreen.h>


class XPT2046 : public CTouchScreenBase
    // Constructed in context of a TFT CScreenDeviceBase (i.e. ili9486).
	// You must call setRotation() at startup and whenever the
	// display device is rotated.
    //
	// Furthermore you must take care to call Update() from
    // the same thread/processor as any UI drawing routines,
    // inasmuch as both use the SPIMaster object which is not
    // otherwise protected.
{
public:

    XPT2046(CSPIMaster *pSPI, u16 width, u16 height);
		// Constructed with knowledge abou the fixed
		// physical width and height in rotation 0
    ~XPT2046();

    void setRotation(u8 rotation)
    {
		m_rotation = rotation;
    }

	virtual void Update(void);

	void setCalibration(u16 *values);
	bool doCalibration(u16 *values);
		// set the Calibration values (minx,maxx,miny,maxy) or
		// perform a calibration UI and return true if it worked.


protected:

    CSPIMaster  		*m_pSPI;

    u8  m_rotation;
    u16 m_width;			// width in the rotated state (not absolute)
    u16 m_height;			// height in the rotated state (not absolute)

    u16 m_lastx;
    u16 m_lasty;
    u16 m_lastz;

	u16 m_min_x;
	u16 m_max_x;
	u16 m_min_y;
	u16 m_max_y;

    u16 transfer16(u8 reg);

};



#endif      // !__xpt2046_h__
