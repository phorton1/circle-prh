#ifndef __ili9486_h__
#define __ili9486_h__

#include <circle/gpiopin.h>
#include <circle/spimaster.h>
#include <ugui/uguicpp.h>


// #define ILI9846_WITH_DEBUG_PIN  

class ILI9846 : public alternateScreenDevice
{
public:
    
    ILI9846(CSPIMaster *pSPI);
    ~ILI9846();

    virtual boolean Initialize (void);
    virtual unsigned getWidth (void) const;
    virtual unsigned getHeight (void) const;

    u8 getRotation() { return m_rotation; }
    void setRotation(u8 rotation);
        // the default rotation is 1
    
    void fillRect(int xs, int ys, int xe, int ye, u16 color565);
    virtual void setPixel(UG_S16 sPosX, UG_S16 sPosY, UG_COLOR Color);

    static UG_RESULT fillFrame( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_COLOR c );
    static UG_RESULT drawLine( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_COLOR c );
    static void *fillArea( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2);
        // FillArea sets up the window and returns a pointer to pushPixel
        // which is then called for each pixel to write it.
    static void pushPixel(UG_COLOR c);
    
protected:
    
	virtual void provideUGOptimizations();
    
private:
    
    static ILI9846 *s_pThis;
    
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
    