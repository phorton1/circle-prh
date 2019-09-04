// from my circle CMiniUartDevice, which was in turn
// copied from dwelch's periph.c bootloader mini uart code

#ifndef _transport_mini_uart_h
#define _transport_mini_uart_h

#include "uartTransportBase.h"
#include <circle/interrupt.h>
#include <circle/gpiopin.h>


class transportMiniUart : public uartTransportBase
{
public:
    
	transportMiniUart(CInterruptSystem *pInterruptSystem=0);
	~transportMiniUart(void);

	virtual void initialize(unsigned nBaudrate = 115200);


private:
    friend class miniTask;
    
	virtual void Write(u8 nChar);

	void IRQHandler(void);
	static void IRQStub(void *pParam);
    
	CInterruptSystem *m_pInterruptSystem;
	boolean m_bIRQConnected;

};


#endif  

