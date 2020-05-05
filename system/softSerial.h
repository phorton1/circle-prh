// softSerial.h
//
// A bit banged implementation of a serial port

#ifndef _softSerial_h
#define _softSerial_h

#include <circle/types.h>
#include <circle/gpiopin.h>
#include <circle/timer.h>


class softSerial 
{
public:

	softSerial(unsigned tx_pin, unsigned rx_pin, unsigned long baud_rate);
	~softSerial(void) {}

	int write(const void *buf, int count);
    void writeByte(u8 c);

	int read (u8 *buf, int count);  // not implemented ... see bootloader/softserial.h and cpp
    
private:
    
    unsigned long m_mhz_per_bit;
    
    CTimer *m_timer;
    CGPIOPin *m_pin_tx;
    CGPIOPin *m_pin_rx;
    
    
    
};  // class softSerial

#endif  // !_softSerial_h

