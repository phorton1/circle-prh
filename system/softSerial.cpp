
#include "softSerial.h"
#include <circle/logger.h>
#include <utils/myUtils.h>
#include <circle/synchronize.h>



#define log_name "soft_serial"


// #define STATE_INIT	1	// init to constant state
// #define STATE0      0   // sent for a zero
// #define STATE1      1	// sent for a one
// #define START_BIT   0	// sent to start a character
// #define STOP_BIT    1	// sent to end a character



softSerial::softSerial(unsigned tx_pin, unsigned rx_pin, unsigned long baud_rate) 
{
	LOG("softSerial ctor",0);
	m_mhz_per_bit = CLOCKHZ/baud_rate;	// CLOCKHZ==1,000,000
	
	m_timer = CTimer::Get();
	m_pin_tx = new CGPIOPin(tx_pin,GPIOModeOutput);
	m_pin_rx = new CGPIOPin(rx_pin,GPIOModeInput);		// GPIOModeInputPullDown
	m_pin_tx->Write(1);
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
		
	m_pin_tx->Write(0);	// write the start bit
	m_timer->usDelay(m_mhz_per_bit);
	
	for (int i=0; i<8; i++)		// LSB first
	{
		m_pin_tx->Write(((c>>i)&0x01));	// write the data bits
		m_timer->usDelay(m_mhz_per_bit);

	}
	m_pin_tx->Write(1);	// write the start bit
	m_timer->usDelay(m_mhz_per_bit);

	LeaveCritical();
}



