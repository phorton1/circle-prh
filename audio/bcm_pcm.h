
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
// With i2s, a block contains both the Left and Right channels
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
// * sizeof(u32) * num_channels.  It is 1024 bytes for i2s stereo
// and 4096 bytes for 8 channel TDM.


#define RAW_AUDIO_BLOCK_SAMPLES   (m_NUM_CHANNELS * AUDIO_BLOCK_SAMPLES)
	// number of u32 samples in am interleaved (raw audio) block
#define RAW_AUDIO_BLOCK_BYTES 	  (m_NUM_CHANNELS * AUDIO_BLOCK_SAMPLES * sizeof(u32))
	// number of bytes in a raw audio block


#define INCLUDE_ACTIVITY_LEDS	1
	// set to 1 to output flashing rx and tx leds on GPIO 23 and 24
	

typedef void voidMethod();


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

	// STATIC INITIALIZATION METHODS
	// these methods may be called statically by the individual
	// teensy i2s control and/or input and output devices during
	// construction. Then init() and start() can be called by any
	// device and everything should work as expected
	
	void setInISR(voidMethod  *in_isr)  { m_inISR = in_isr; }
	void setOutISR(voidMethod *out_isr) { m_outISR = out_isr; }
		// these are called by the input and output devices
		
	void static_init(
		bool		as_slave,
		u32   		sample_rate,
		u8    		sample_size,
		u8			num_channels,
		u8    		channel_width,
		u8			channel1_offset = 1,
		u8			channel2_offset = 1,
		u8			invert_bclk = 0,
		u8          invert_fclk = 0,
		voidMethod  *startClockMethod = 0)
	{
		m_as_slave = as_slave;		
		m_SAMPLE_RATE = sample_rate;
		m_SAMPLE_SIZE = sample_size;
		m_NUM_CHANNELS = num_channels;
		m_CHANNEL_WIDTH = channel_width;
		m_CHANNEL1_OFFSET = channel1_offset;
		m_CHANNEL2_OFFSET = channel1_offset;
		m_INVERT_BCLK = invert_bclk;
		m_INVERT_FCLK = invert_fclk;
		m_startClock = startClockMethod;
	}
	
	// static_init() sets up the frame and DMA for the bcm PCM/I2S peripheral.
	// It may be called by the AudioControl device or, if there is no AudioControl
	// device, an AudioStream, as needed.
	//
	//     as_slave = whether the bcm_pcm (rpi) will act as the
	//        master, or slave, in communicating with the other
	//        device.  If !as_slave the rpi provides the BCLK
	//        and FCLK.
	//
	//        This *may* need to be made bitwise for tdm which
	//        independently allows for receiving or transmitting
	//        BCLK and FCLK.   In other words, it is possible for
	//        the bcm_pcm to, for example, generate BCLK, but respond
	//        as a "slave" to the FCLK generated by the other device,
	//        and vice versa. I am still working on tdm, and I am not
	//        sure at this point if I will need that functionality.
	//
	//        So, at this time, as_slave means that the bcm_pcm responds
	//        to the remote FCLK and BCLK, and that !as_slave means it
	//        creates FCLK and BCLK.
	//
	//     sample_rate = Fs = the number of FULL n-channel samples per second.
	//
	//        Each "sample" consists num_channels "raw" samples.
	//        For i2s num_channels == 2, so, at 44100, there
	//        are 88200 raw samples per second. For 8 channel TDM
	//        there are 8*44100=352800 raw samples per second.
	//
	//     sample_size = the number of bits per raw sample
	//
	//        We (the teensy audio library) generally uses s16
	//        raw samples. Due to limitations in the bcm DMA,
	//        these are always buffered into u32s by this object
	//        and debuffered into arrays of s16's by the teensy
	//        code.
	//
	//     num_channels = number of channels per FULL frame
	//
	//        For i2s this is 2.  For TDM it depends on the devuce,
	//        My implementation of the cs42448 audioInjector octo
	//        device uses uses 8 channel TDM in accordance with the
	//        device specs.
	//
	//        A raw audio block contains AUDIO_BLOCK_SAMPLES * num_channels
	//        "raw" samples stored in u32s. The bcm_pcm DMA always fills
	//        up a "full" raw audio block at a time during interrupt
	//        processing, so this is 2 * 128 * sizeof(u32) = 1024 bytes
	//        for an i2s stereo setup, and 8 * 128 * sizeof(u32) = 4096
	//        bytes for my 8 channel TDM implementation.
	//
	//    channel_width = number of BCLKs per LRCLK
	//
	//        The channel width of most devices, i2s or tdm, is 32,
	//        so as to accomodate 16,24, and 32 bit sample sizes.
	//        The only case I have seen that is not 32 bits is the
	//        teensy i2squad device, running on the teensy, which
	//        uses the teensy's dual i2s devices to make two simultaneous
	//        i2s ports running with 16 bit channel widths.
	//
	//        All that is basically necessary is that the channel length
	//        is larger or equal than the sample size, although the I2s spec
	//        even allows for channel lengths less than the sample size
	//        by stating that subsequent missing bits shall be set to 0.
	//
	//    channel1_offset & channel2_offset
	//
	//        How many BCLK cycles to skip at the start of a frame, and
	//        upon the LR switch after channel_width BLCKS, respectively.
	//        The default values for i2s are to skip one BCLK at the
	//        the start of the frame, and one after the LR change, but
	//        certain devices may require other fine tuning of the
	//        channel offsets. For instance the Octo actually uses 0,0
	//        grabbing bits on the 0th BCLK cycle.
	//
	//     invert_bclk and invert_fclk
	//
	//        The sense of the BCLK and FCLK signals can be reversed.
	//        Needed for certain devices, these *should* not be needed
	//        for correct i2c.
	//
	// Together these vsriables determine the RAW_AUDIO_BLOCK_SIZE, DMA setup
	// and, if !as_slave, the BCLK frequency.
	//
	//    startClockMethod = a static void method to be called to start
	//        the master clock. This method will be called from start()
	//        after everything is ready, just before we turn on the DMA.
	//        It is needed, for example, for the Octo, which must start
	//        its clock at the right time in order to be synchronized.

	
	// DYNAMIC INITIALIZATION
	//
	// This call sets up the memory buffers, DMA control blocks,
	// and does the basic initialization of the bcm pcm (i2s) device.
	// It must be called AFTER the kernel is initialized and before start().
	// It may be called more than once, but will only really start things
	// on the first call, so it can be called from AudioStream begin()
	// methods.
	// 
	// By the time init() is called we must know if the bcm_pcm will
	// be used for one way or two way i2s, as there is only one combined
	// setup for the pcm peripheral. If there is only one direction, only
	// one call to setInISR() will have occurred. If it is bi-directional,
	// then both isr's will be set.
	
	void init();
	void terminate();
	
	// once the device is initialized, you call start() to begin
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

	bool       	m_as_slave;
	u32 	   	m_SAMPLE_RATE;
	u8		   	m_SAMPLE_SIZE;
	u8			m_NUM_CHANNELS;
	u8         	m_CHANNEL_WIDTH;
	u8			m_CHANNEL1_OFFSET;
	u8			m_CHANNEL2_OFFSET;
	u8		    m_INVERT_BCLK;
	u8			m_INVERT_FCLK;

	voidMethod 	*m_inISR;
	voidMethod 	*m_outISR;
	voidMethod  *m_startClock;

	void initBuffers();
	uint32_t *allocateRawAudioBlock(u8 **allocBlock);
	
	void initDMA(
		bool output,
		TDMAControlBlock *pb,
		unsigned id,
		unsigned other_id);
	void initFrame();
	void stopPCM();

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

	uint32_t *m_inBuffer[2];
	uint32_t *m_outBuffer[2];
	u8 *m_allocInBuffer[2];
	u8 *m_allocOutBuffer[2];
	
	CSpinLock 				m_SpinLock;

	friend class statusScreen;
	
	u32	in_block_count;
	u32	in_other_count;
	u32	in_wrong_count;
	u32	out_block_count;
	u32	out_other_count;
	u32	out_wrong_count;
	u32 underflow_count;
	u32 overflow_count;
};


extern BCM_PCM bcm_pcm;

#endif	
