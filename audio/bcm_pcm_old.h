// A somewhat generalized interface to the bcm2835
// "pcm" module which does i2s and tdm communications.
//
// The teensy audio library has a number of instances of duplicated
// code in the initialization of i2s devices for master, slave,
// channel width, etc, so this API attempts to consolidate those
// pieces into a single configuration call.


#ifndef _bcm_pcm_h_
#define _bcm_pcm_h_

#include <Arduino.h>
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>
#include <circle/interrupt.h>
#include <circle/dmachannel.h>

#define WITH_ACTIVITY_LEDS  1


#define AUDIO_IN    0x0001
#define AUDIO_OUT   0x0002

typedef void isr_method();



class BCM_PCM
{
public:

	BCM_PCM();
	~BCM_PCM();

	void init(
		u32   sample_rate,
		u8    sample_size,
		u8    channel_length,
		bool  as_slave,
		u8    direction = AUDIO_IN | AUDIO_OUT);
	
	void begin(
		bool   		output,
		isr_method 	*isr,
		u16			len,
		uint32_t	*buf0,
		uint32_t 	*buf1);
	
	uint32_t *getInBuffer();
	uint32_t *getOutBuffer();
	void toggleIn();
	void toggleOut();
	
	void clearIRQ(bool output);
	
private:

	u32 		m_SAMPLE_RATE;
	u8			m_SAMPLE_SIZE;
	u8  		m_CHANNEL_LENGTH;
	bool		m_as_slave;
	u8			m_direction;

	bool		m_initialized;
	CInterruptSystem *m_pInterruptSystem;
	bool 	   	m_bInIRQConnected;
	bool 	   	m_bOutIRQConnected;

	CGPIOPin   	m_BCLK;
	CGPIOPin   	m_FCLK;
	CGPIOPin   	m_RXD;
	CGPIOPin   	m_TXD;
	CGPIOClock 	m_BitClock;
	
	#if WITH_ACTIVITY_LEDS
		CGPIOPin   	m_rxActive;
		CGPIOPin   	m_txActive;
	#endif

	unsigned 	m_nDMAInChannel;
	unsigned 	m_nDMAOutChannel;
	
	u8 m_inToggle;
	u8 m_outToggle;
	uint16_t  m_dma_len;
	uint32_t *m_inBuffer[2];
	uint32_t *m_outBuffer[2];
	
	TDMAControlBlock m_inControlBlock[2]  __attribute__ ((aligned (4)));
	TDMAControlBlock m_outControlBlock[2] __attribute__ ((aligned (4)));
	
	void initDMA(bool output, unsigned id);
	void initI2s();
	
};


extern BCM_PCM bcm_pcm;


#endif

