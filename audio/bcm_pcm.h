
#ifndef _bcm_pcm_h_
#define _bcm_pcm_h_

#include <circle/types.h>
#include <circle/interrupt.h>
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>
#include <circle/spinlock.h>
#include <circle/dmachannel.h>
#include "AudioStream.h"


// Teensy definition:
// AUDIO_BLOCK_SAMPLES  128

// There are various sized "blocks" and buffers in the system.
//
// By and large the teensy passes around buffers that contain
// 128 s16 samples for one mono channel. These are the teensy
// audio blocks created by AudioStream::allocate(), which can
// be connected in linked lists, and are processed by update()
// methods with the teensy API.  So the "size" of a teensy
// audio block buffer, in bytes, is AUDIO_BLOCK_SAMPLES * 2
// = 256 bytes
//
// With i2s a block contains both the Left and Right channels
// with the data interleaved (LRLRLR). On the bcm283x, using dma,
// the individual samples will be 32 bits wide, regardless of the
// chosen is2s sample size or channel width. This is because the
// bcm DMA can only "move" a minimum of 32 bits per operation
// for each programmed access. There is no way to do a DMA on the
// pcm from a fifo to memory with 8 or 16 bit sized buffers.
//
// Therefore the "raw" audio blocks passed to and from bcm i2s
// using dma consist of 32 bit values AND they are interleaved,
// and the size of a "raw" audio block is AUDIO_BLOCK_SAMPLES
// * sizeof(u32) * 2_for_interleaving = 1024 bytes.


#define RAW_AUDIO_BLOCK_SAMPLES   (2 * AUDIO_BLOCK_SAMPLES)
	// number of u32 samples in am interleaved (raw audio) block
#define RAW_AUDIO_BLOCK_BYTES 	  (RAW_AUDIO_BLOCK_SAMPLES * sizeof(u32))
	// number of bytes in a raw audio block


#define INCLUDE_ACTIVITY_LEDS	1
	// set to 1 to output flashing rx and tx leds on GPIO 23 and 24
	

typedef void audioIRQ();


enum bcmSoundState
{
	bcmSoundIdle,
	bcmSoundError,
	bcmSoundUnknown,
	bcmSoundRunning,
	bcmSoundCancelled,
	bcmSoundTerminating
};




class BCM_PCM
{
public:

	BCM_PCM();
	~BCM_PCM();

	// these methods may be called statically by the individual
	// teensy i2s input and output devices during construction.
	// then init() and start() can be called by either device
	// and it will cause both directions to be setup as needed.
	
	void setInISR(audioIRQ 	*in_isr)  { m_inISR = in_isr; }
	void setOutISR(audioIRQ *out_isr) { m_outISR = out_isr; }
	
	// by the time the teensy calls init(), we must know if the
	// bcm_pcm will be used for one way or two way i2s, as
	// there is only one combined setup for the pcm peripheral.
	// If there is only one channel, only one isr will be set.
	// If it is bi-directional, then both isr's will be set.
	
	void init(
		u32   		sample_rate,
		u8    		sample_size,
		u8    		channel_length,
		bool  		as_slave);
	void terminate();
	
	// once the device is initialize, you call start() to begin
	// receiving or sending buffers ... the device can be started
	// or stopped multiple times
	
	void start();
	void stop();

	// you may query the state of the device to see if it is running
	
	bcmSoundState getState() { return m_state; };

	// the following methods can be called from client's in_isr()
	// and out_isr() methods.
	//
	// getInBuffer() returns the input buffer that was just completed
	// by DMA. A second buffer is currently being filled in. in_isr()
	// must complete in time to pass the just completed buffer back
	// to the DMA as the NEXt buffer before the next interrupt.
	
	uint32_t *getInBuffer()		{ return m_inBuffer[m_inToggle]; }
	uint32_t *getOutBuffer()	{ return m_outBuffer[m_outToggle]; }
	unsigned getInToggle()		{ return m_inToggle; }
	unsigned getOutToggle()		{ return m_outToggle; }
	
	// likewise, the client's out_isr() method calls getOutBuffer()
	// to get a buffer, which it can fill in.  It needs to complete
	// in time for the buffer to be handed to the DMA for output
	// BEFORE the next DMA interrupt
	// 	
	// Otherwise, the client in_isr() and out_isr() methods do not
	// have to do any bcm_pcm specific interrupt managment.  The pending
	// irqs are cleared in a local isrHandler before the teensy in_isr()
	// or out_isr() methods are called.

	
private:

	u32 	   	m_SAMPLE_RATE;
	u8		   	m_SAMPLE_SIZE;
	u8         	m_CHANNEL_LENGTH;
	bool       	m_as_slave;
	audioIRQ 	*m_inISR;
	audioIRQ 	*m_outISR;

	void initBuffers();
	void initDMA(
		bool output,
		TDMAControlBlock *pb,
		unsigned id,
		unsigned other_id);
	void initI2S();
	void startI2S();
	void stopI2S();

	static void audioInIRQStub(void *pParam);
	static void audioOutIRQStub(void *pParam);
	void audioInIRQ(void);
	void audioOutIRQ(void);
	void updateInput(bool cold);
	void updateOutput(bool cold);

	CInterruptSystem *m_pInterruptSystem;
	
	CGPIOPin   m_BCLK;
	CGPIOPin   m_FCLK;
	CGPIOPin   m_RXD;
	CGPIOPin   m_TXD;
	CGPIOClock m_BitClock;
	
	#if INCLUDE_ACTIVITY_LEDS
		CGPIOPin   m_RX_ACTIVE;
		CGPIOPin   m_TX_ACTIVE;
	#endif
	
	volatile bcmSoundState 	m_state;
	bool                    m_initialized;
	bool 					m_bInIRQConnected;
	bool 					m_bOutIRQConnected;
	
	unsigned 				m_nDMAInChannel;
	unsigned 				m_nDMAOutChannel;
	
	unsigned 				m_inToggle;		// 0 or 1
	unsigned 				m_outToggle;	// 0 or 1
	TDMAControlBlock 		m_inControlBlock[2]   __attribute__ ((aligned (32)));
	TDMAControlBlock 		m_outControlBlock[2]  __attribute__ ((aligned (32)));
																 
	uint32_t m_inBuffer[2][RAW_AUDIO_BLOCK_SAMPLES] __attribute__ ((aligned (4)));
	uint32_t m_outBuffer[2][RAW_AUDIO_BLOCK_SAMPLES] __attribute__ ((aligned (4)));
	
	CSpinLock 				m_SpinLock;

	u32			in_block_count;
	u32 		out_block_count;
	u32         underflow_count;
	u32         overflow_count;
};


extern BCM_PCM bcm_pcm;

#endif	
