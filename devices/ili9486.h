#ifndef __ili9486_h__
#define __ili9486_h__

#include "ili_base.h"


class ILI9846 : public ILIBASE
{
public:

    ILI9846(ILISPI_CLASS *pSPI);
    ~ILI9846();

    virtual boolean Initialize(void) override;

private:

	CGPIOPin    m_pinRESET;

	virtual void color565ToBuf(u16 color, u8 *buf) override;

};


#endif      // !__ili9486_h__
