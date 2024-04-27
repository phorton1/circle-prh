//
// spimaster.h
//

#ifndef _bangspi_h
#define _bangspi_h

#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CBangSPI
{
public:
	CBangSPI (unsigned nClockSpeed = 500000, unsigned CPOL = 0, unsigned CPHA = 0);
	~CBangSPI (void);

	boolean Initialize (void);

	void SetClock (unsigned nClockSpeed);			// in Hz
	int Read (unsigned nChipSelect, void *pBuffer, unsigned nCount);
	int Write (unsigned nChipSelect, const void *pBuffer, unsigned nCount);
	int WriteRead (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer, unsigned nCount);

private:
	unsigned m_nClockSpeed;
	unsigned m_CPOL;
	unsigned m_CPHA;

	CGPIOPin m_SCLK;
	CGPIOPin m_MOSI;
	CGPIOPin m_MISO;
	CGPIOPin m_CE0;
	CGPIOPin m_CE1;

	CSpinLock m_SpinLock;

	void setCS(u8 num, u8 val);
	void delayClock(u8 num, u8 val);

};

#endif
