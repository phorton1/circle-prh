//
// bangspi.cpp

#include "bangspi.h"
#include <circle/timer.h>



CBangSPI::CBangSPI (unsigned nClockSpeed, unsigned CPOL, unsigned CPHA)
:	m_nClockSpeed (nClockSpeed),
	m_CPOL (CPOL),
	m_CPHA (CPHA),
	m_SCLK (11, GPIOModeOutput),	// GPIOModeAlternateFunction0),
	m_MOSI (10, GPIOModeOutput),	// GPIOModeAlternateFunction0),
	m_MISO ( 9, GPIOModeInput),		// GPIOModeAlternateFunction0),
	m_CE0  ( 8, GPIOModeOutput),	// GPIOModeAlternateFunction0),
	m_CE1  ( 7, GPIOModeOutput),	// GPIOModeAlternateFunction0),
	m_SpinLock (TASK_LEVEL)
{
}

CBangSPI::~CBangSPI (void)
{
}

boolean CBangSPI::Initialize (void)
{
	m_SCLK.Write(0);
	m_MOSI.Write(0);
	m_CE0.Write(1);
	m_CE1.Write(1);
	return true;
}

void CBangSPI::SetClock (unsigned nClockSpeed)
{
	m_nClockSpeed = nClockSpeed;
}


int CBangSPI::Read (unsigned nChipSelect, void *pBuffer, unsigned nCount)
{
	return WriteRead (nChipSelect, 0, pBuffer, nCount);
}

int CBangSPI::Write (unsigned nChipSelect, const void *pBuffer, unsigned nCount)
{
	return WriteRead (nChipSelect, pBuffer, 0, nCount);
}





void CBangSPI::setCS(u8 num, u8 val)
{
	if (num)
		m_CE1.Write(val);
	else
		m_CE0.Write(val);
}


volatile int dummy = 0;

void CBangSPI::delayClock(u8 num, u8 val)
{
	if (num)
		CTimer::Get()->usDelay(val);
	else
	{
		for (u8 i=0; i<val; i++)
		{
			// for (int j=0; j<2; j++)
			// {
				dummy++;
			// }
		}
	}
}




int CBangSPI::WriteRead (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer, unsigned nCount)
{
	const u8 *op = (const u8 *) pWriteBuffer;
	u8 *ip = (u8 *) pReadBuffer;
	m_SpinLock.Acquire ();

	setCS(nChipSelect, 0);
	delayClock(nChipSelect, 3);
	for (u8 i=0; i<nCount; i++)
	{
		u8 in_byte = 0;
		u8 out_byte = op ? *op++ : 0;

		for (u8 j=0; j<8; j++)
		{
			u8 out_bit = out_byte & (1 << (7-j));
			m_MOSI.Write(out_bit? 1 : 0);
				delayClock(nChipSelect, 1);
			u8 in_bit = m_MISO.Read();
			m_SCLK.Write(1);
				delayClock(nChipSelect, 1);
			m_SCLK.Write(0);

			in_byte |= (in_bit << (7-j));
		}

		if (ip)
			*ip++ = in_byte;
	}
	delayClock(nChipSelect, 3);
	setCS(nChipSelect, 1);
	
	m_SpinLock.Release ();
	return (int) nCount;
}
