#ifndef __xpt2046_h__
#define __xpt2046_h__


#include <circle/spimaster.h>
#include <circle/screen.h>
#include <circle/input/touchscreen.h>
#include <fatfs/ff.h>
#include "ili_base.h"


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

    XPT2046(ILISPI_CLASS *pSPI, ILIBASE *pTFT);
		// Constructed with knowledge abou the fixed
		// physical width and height in rotation 0
    ~XPT2046();

    void setRotation(u8 rotation);
	virtual void Update(void) override;

	void Initialize(FATFS *pFileSystem);
	void startCalibration();
	int inCalibration() { return m_calibration_phase; }

protected:

    ILISPI_CLASS *m_pSPI;
	ILIBASE    *m_pTFT;
	FATFS 	   *m_pFileSystem;

    u8  m_rotation;
    u16 m_width;			// width in the rotated state (not absolute)
    u16 m_height;			// height in the rotated state (not absolute)

    u16 m_lastx;
    u16 m_lasty;
    u16 m_lastz;

	s16 m_min_x;
	s16 m_max_x;
	s16 m_min_y;
	s16 m_max_y;

    u16 transfer16(u8 reg);

	// calibration stuff

	int m_calibration_phase;
	u32 m_calibration_time;
	u32 m_display_time;

	s16 m_save_min_x;
	s16 m_save_max_x;
	s16 m_save_min_y;
	s16 m_save_max_y;

	int m_button_pressed;

	void endCalibration(bool ok);
	void writeCalibration();
	int printCentered(int size, int start_y, const char *str);
	void showButton(int num, bool down);
	void calibMove(u16 state, u16 x, u16 y);

};



#endif      // !__xpt2046_h__
