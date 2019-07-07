#ifndef __ili9486_h__
#define __ili9486_h__

#include <circle/gpiopin.h>
#include <circle/spimaster.h>
#include <circle/screen.h>


// #define ILI9846_WITH_DEBUG_PIN  

class ILI9846 : public CScreenDeviceBase
{
public:
    
    ILI9846(CSPIMaster *pSPI);
    ~ILI9846();

    // CScreenDeviceBase overrides
    
    /* virtual */ boolean Initialize(void);
	/* virtual */ virtual void InitializeUI(void *pUI, DriverRegisterFxn registerFxn);
    /* virtual */ unsigned GetWidth(void) const;
    /* virtual */ unsigned GetHeight(void) const;
    /* virtual */ void SetPixel(unsigned x, unsigned y, u16 color);
    /* virtual */ u16 GetPixel(unsigned x, unsigned y) { return 0; }    

    u8 getRotation() { return m_rotation; }
    void setRotation(u8 rotation);
        // the default rotation is 1
    void fillRect(int xs, int ys, int xe, int ye, u16 color);

    // static methods that can be registered with ugui
    // FillArea sets up the window and returns a pointer to pushPixel
    // which is then called for each pixel to write it.
    
    static s8 staticFillFrame(void *pThis, s16 x1, s16 y1, s16 x2, s16 y2, u16 color);
    static void *staticFillArea(void *pThis, s16 x1, s16 y1, s16 x2, s16 y2);
    static void staticPushPixel(void *pThis, u16 color);
    
    
private:
    
    CSPIMaster  *m_pSPI;
        
    CGPIOPin    m_pinCD;
    CGPIOPin    m_pinRESET;
    
    #ifdef ILI9846_WITH_DEBUG_PIN
        CGPIOPin    m_pinDebug;
    #endif
    
    u8 m_rotation;
    
    void write(u8 *data, u16 len);
    void writeCommand(u8 command, u8 *data, u16 len);
    void setWindow(int xs, int ys, int xe, int ye);
    
};

#endif      // !__ili9486_h__
    