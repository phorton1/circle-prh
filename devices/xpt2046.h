#ifndef __xpt2046_h__
#define __xpt2046_h__

#include <circle/gpiopin.h>
#include <circle/spimaster.h>
#include <circle/input/touchscreen.h>


class XPT2046 : public CTouchScreenBase
    // Typically used with another device (i.e. ili9486), you
    // must call setDimensions(), and setRotation() before
    // initialization. Thereafter you must call setRotation()
    // whenever the correponding physical device is rotated.
    // Furthermore you must take care to call Update() from
    // the same thread/processor as any UI drawing routines,
    // inasmuch as both use the SPIMaster object which is not
    // otherwise protected.
{
public:
    
    XPT2046(CSPIMaster *pSPI);
    ~XPT2046();

    void setDimensions(u16 height, u16 width)
    {
        m_height = height;
        m_width = width;
    }
    void setRotation(u8 rotation)   { m_rotation = rotation; }
    
	virtual void Update(void);

protected:    

    CSPIMaster  *m_pSPI;
    
    CGPIOPin    m_pinCS_TOUCH;
    
    u8  m_rotation;
    u16 m_width;
    u16 m_height;
    u16 m_lastx;
    u16 m_lasty;
    u16 m_lastz;
    
    u16 transfer16(u8 reg);
    
};



#endif      // !__xpt2046_h__
    