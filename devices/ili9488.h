#ifndef __ili9488_h__
#define __ili9488_h__

#include "ili_base.h"


class ILI9488 : public ILIBASE
{
public:

    ILI9488(ILISPI_CLASS *pSPI);
    ~ILI9488();

    virtual boolean Initialize(void) override;

private:

	virtual void color565ToBuf(u16 color, u8 *buf) override;

};

#endif      // !__ili9488_h__
