
#include "softSerial.h"
#include <circle/logger.h>
#include <circle/gpiopin.h>
#include <circle/timer.h>
#include <utils/myUtils.h>
#include <circle/synchronize.h>


#define PIN_TX  8
#define PIN_RX  9
	// needs to be two GPIO's that are currently
	// not used for anything

#define log_name "soft_serial"

CGPIOPin *pin_tx = 0;
CGPIOPin *pin_rx = 0;
CTimer *timer = 0;


#define STATE_INIT	1	// init to constant state
#define STATE0      0   // sent for a zero
#define STATE1      1	// sent for a one
#define START_BIT   0	// sent to start a character
#define STOP_BIT    1	// sent to end a character


softSerial::softSerial(unsigned long baud_rate) 
{
	LOG("softSerial ctor",0);
	m_mhz_per_bit = 1000000/baud_rate;
	timer = CTimer::Get();
	pin_tx = new CGPIOPin(PIN_TX,GPIOModeOutput);
	pin_rx = new CGPIOPin(PIN_RX,GPIOModeInput);		// GPIOModeInputPullDown
	pin_tx->Write(STATE_INIT);
	delay(20);
}



int softSerial::read(u8 *buf, int count)	// not implemented
{
	LOG("read %d bytes",count);
	return 0;
}


int softSerial::write(const void *buf, int count)
{
	const unsigned char *bbb = (const unsigned char *) buf;
	display_bytes("write",bbb,count);
	for (int i=0; i<count; i++)
	{
		writeByte(bbb[i]);
	}
	return count;
}


void softSerial::writeByte(u8 c)
{
	EnterCritical ();
		
	pin_tx->Write(START_BIT);	// write the start bit
	timer->usDelay(m_mhz_per_bit);
	
	for (int i=0; i<8; i++)		// LSB first
	{
		pin_tx->Write(((c>>i)&0x01)?STATE1:STATE0);	// write the data bits
		timer->usDelay(m_mhz_per_bit);

	}
	pin_tx->Write(STOP_BIT);	// write the start bit
	timer->usDelay(m_mhz_per_bit);

	LeaveCritical();
}



