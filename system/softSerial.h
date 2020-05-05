// softSerial.h
//
// A bit banged implementation of a serial port

#ifndef _softSerial_h
#define _softSerial_h

#include <circle/types.h>

class softSerial 
{
public:

	softSerial(unsigned long baud_rate);
	~softSerial(void) {}

	int write(const void *buf, int count);
    void writeByte(u8 c);

	int read (u8 *buf, int count);  // not implemented ... see bootloader/softserial.h and cpp
    
private:
    
    unsigned long m_mhz_per_bit;
    
    
};  // class softSerial

#endif  // !_softSerial_h

