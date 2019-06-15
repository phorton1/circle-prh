// nuther panic checking .. something is weird here
// like the serial port interrupts are hooked to the
// dma interrupts ... it's a bitch just to get the 
// dma irq routines called once ... but if I include
// a certain printf() in the methods, then it starts
// looping and flashing like the DMA interrupts are 
// working, though I have my doubts and think it is
// probably the serial interrupts getting munged.
// 
// In addition there seems to be some initialization
// that is not being performed correctly, as sometimes
// it doesn't happen on a freshly booted rPi until AFTER 
// I re-run my old program.
// 
// Something is smelly in denmark, and I have to 
// figure it out.  May need to go back to basics
// and a single kernel implementation of i2s for
// testing.

#include "BCM_PCM.h"
#include "bcm_pcm_defines.h"
#include <circle/util.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/alloc.h>
#include <circle/logger.h>
#include <circle/machineinfo.h>




#if BCM_KNOWS_OCTO
	// hopefully vestigial code
	#include "control_cs42448.h"
	extern AudioControlCS42448 *pControl;
#endif



#define OPTIMIZED_IRQS	0
	// removes uncessary asserts and register accesses
	// from the irq routines (while leaving asserts in
	// the reset of the code)

#define log_name "bcm_pcm"

#if 0
	#define PCM_LOG(f,...)           CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
#else
	#define PCM_LOG(...)
#endif


#define PIN_BCLK	   18		// The bit clock
#define PIN_FCLK       19		// The L/R frame clock
#define PIN_RXD		   20       // I2S receive data
#define PIN_TXD		   21		// I2S transmit data

#define PIN_RX_ACTIVE  12		// a LED to show recv activity
#define PIN_TX_ACTIVE  16		// a LED to show xmit activity


// This CLOCK_FREQ is used if we are the i2s master.

#define CLOCK_FREQ		   500000000


// Test Routings and latency
// Consider the time it takes to clock one RAW_AUDIO_BLOCK through
// the i2s device as "one unite".  We'll call it 3ms for now.

#define TEST_SHORT_CIRCUIT_IN_TO_OUT   0
	// If 1, the input and output DMA's will use the same pair
	// of physical buffers.  Since the input DMA is started first
	// it just happens that the output DMA always has some bytes
	// available to it.  LIKELY has sub 3ms latency, and is
	// guaranteed to have sub-6ms latency, or it wouldn't work
	// without more buffers!

#define TEST_ROUTE_IN_TO_OUT    0
	// If 1 the "teensy client" isr routines will not be called.
	// A pair of buffers are used to move data from the input DMA
	// to the output DMA.  PROBABLY has sub 6ms latancy.

#define OUTPUT_DISTINCTIVE_PATTERN  0
	// Used for debuging frames.  If set,
	// will output a distinctive pattern of samples.


#if TEST_ROUTE_IN_TO_OUT
	uint32_t *test_buffer[2];
	u8  *test_alloc[2];
#endif


// static instance

BCM_PCM bcm_pcm;


//--------------------------------
// ctor
//--------------------------------

BCM_PCM::BCM_PCM() :
	m_pInterruptSystem(0),
	m_BCLK(PIN_BCLK, GPIOModeAlternateFunction0),	// GPIOModeOutput GPIOModeInput	GPIOModeAlternateFunction0
	m_FCLK(PIN_FCLK, GPIOModeAlternateFunction0),
	m_RXD(PIN_RXD, 	 GPIOModeAlternateFunction0),
	m_TXD(PIN_TXD, 	 GPIOModeAlternateFunction0),
	m_BitClock(GPIOClockPCM, GPIOClockSourcePLLD),
		// GPIOClockSourceOscillator = 1,		// 19.2 MHz
		// GPIOClockSourcePLLC       = 5,		// 1000 MHz (changes with overclock settings)
		// GPIOClockSourcePLLD       = 6,		// 500 MHz hence the #define CLOCK_FREQ 500000000
		// GPIOClockSourceHDMI       = 7		// 216 MHz
	#if INCLUDE_ACTIVITY_LEDS		
		m_RX_ACTIVE(PIN_RX_ACTIVE, GPIOModeOutput),
		m_TX_ACTIVE(PIN_TX_ACTIVE, GPIOModeOutput),
	#endif
	m_nDMAInChannel(CMachineInfo::Get()->AllocateDMAChannel(DMA_CHANNEL_LITE)),
	m_nDMAOutChannel(CMachineInfo::Get()->AllocateDMAChannel(DMA_CHANNEL_LITE))
{
	m_as_slave = false;
	m_SAMPLE_RATE = 0;
	m_SAMPLE_SIZE = 0;
	m_NUM_CHANNELS = 0;
	m_CHANNEL_WIDTH = 0;
	
	// we rely on bss initialization to set these members
	// of the static bcm_pcm object to zero. Otherwise,
	// it *may* happen that the ctor of this is called
	// AFTER the ctor's of the teensy objects, and the
	// stashed pointers get wiped out ...
	// 
	// m_inISR = 0;
	// m_outISR = 0;
	
	m_state = bcmSoundIdle;
	m_initialized = false;
	m_bInIRQConnected = false;
	m_bOutIRQConnected = false;
	
	m_inToggle = 0;
	m_outToggle = 0;
	
	for (u8 i=0; i<2; i++)
	{
		m_inBuffer[i] = 0;
		m_outBuffer[i] = 0;
		m_allocInBuffer[i] = 0;
		m_allocOutBuffer[i] = 0;
	}
	
	wrong_irq_count	= 0;
	in_block_count 	= 0;
	out_block_count = 0;
	underflow_count = 0;
	overflow_count 	= 0;	
	
	#if INCLUDE_ACTIVITY_LEDS		
		m_RX_ACTIVE.Write(1);
		m_TX_ACTIVE.Write(1);
	#endif	
}

	
	
BCM_PCM::~BCM_PCM()
{
	if (m_initialized)
		terminate();
	m_pInterruptSystem = 0;
	
	for (int i=0; i<2; i++)
	{
		if (m_allocInBuffer[i])
			delete m_allocInBuffer[i];
		if (m_allocOutBuffer[i])
			delete m_allocOutBuffer[i];
		#if TEST_ROUTE_IN_TO_OUT
			if (test_alloc[i])
				delete test_alloc[i];
		#endif

		m_inBuffer[i] = 0;
		m_outBuffer[i] = 0;
		m_allocInBuffer[i] = 0;
		m_allocOutBuffer[i] = 0;
	}
}


void BCM_PCM::initBuffers()
{
	m_inToggle = 0;
	m_outToggle = 0;
	
	for (int i=0; i<2; i++)
	{
		if (!m_inBuffer[i])
			m_inBuffer[i] = allocateRawAudioBlock(&m_allocInBuffer[i]);
		if (!m_outBuffer[i])
			m_outBuffer[i] = allocateRawAudioBlock(&m_allocOutBuffer[i]);
			
		#if TEST_ROUTE_IN_TO_OUT
			if (!test_buffer[i])
				test_buffer[i] = allocateRawAudioBlock(&test_alloc[i]);
		#endif
			
		memset(m_inBuffer[i],0,RAW_AUDIO_BLOCK_BYTES);
		memset(m_outBuffer[i],0,RAW_AUDIO_BLOCK_BYTES);
	}
	
	wrong_irq_count	= 0;
	in_block_count 	= 0;
	out_block_count = 0;
	underflow_count = 0;
	overflow_count 	= 0;
}


uint32_t *BCM_PCM::allocateRawAudioBlock(u8 **allocBlock)
	// allocate a raw audio block.
	// Note that these are passed to DMA and MUST be on 32 bit boundaries.,
	// so we over-allocate the memory into allocBlock* and then take the 32
	// bit boundry address with that.
{
	assert(allocBlock);
	u8 *buf = (u8 *) malloc(RAW_AUDIO_BLOCK_BYTES + 3);
	assert(buf);
	u32 addr = (u32) buf;
	uint32_t *rslt = (uint32_t *) (addr & 0xfffffff0);
	PCM_LOG("allocateRawAudioBlock(0x%08x)=0x%08x",(u32)buf,(u32)rslt);
	assert(rslt);
	*allocBlock	= buf;
	return rslt;
}


//-----------------------------
// initialization
//-----------------------------

	
void BCM_PCM::init(
		bool  	  as_slave,
		u32   	  sample_rate,
		u8    	  sample_size,
		u8		  num_channels,
		u8    	  channel_width)
{
	assert(m_inISR || m_outISR);
	// channel_width = 24;

	if (m_state >= bcmSoundRunning)
	{
		PCM_LOG("BCM_PCM::init(%d) returning because state=%d",as_slave,m_state);
		return;		
	}
	
	m_as_slave = as_slave;		
	m_SAMPLE_RATE = sample_rate;
	m_SAMPLE_SIZE = sample_size;
	m_NUM_CHANNELS = num_channels;
	m_CHANNEL_WIDTH = channel_width;
		
	m_pInterruptSystem = CInterruptSystem::Get();
	assert(m_pInterruptSystem);

	#if INCLUDE_ACTIVITY_LEDS		
		m_RX_ACTIVE.Write(0);
		m_TX_ACTIVE.Write(0);
	#endif
		
	LOG("init(%d,%d,%d,%d,%d,%s) ...",
		m_as_slave,
		m_SAMPLE_RATE,
		m_SAMPLE_SIZE,
		m_NUM_CHANNELS,
		m_CHANNEL_WIDTH,
		((u32)m_inISR) & ((u32)m_outISR) ? "BOTH" :
		((u32)m_inISR) ? "INPUT" :
		((u32)m_outISR) ? "OUTPUT" :
		"ERROR - no isr specified in init!");
	
	PCM_LOG("inDMAChannel=%d outDMAChannel=%d",
		m_nDMAInChannel,
		m_nDMAOutChannel);
	PCM_LOG("m_inControlBlock[0]=0x%08x",&m_inControlBlock[0]);
	PCM_LOG("m_inControlBlock[1]=0x%08x",&m_inControlBlock[1]);
	PCM_LOG("m_outControlBlock[0]=0x%08x",&m_outControlBlock[0]);
	PCM_LOG("m_outControlBlock[1]=0x%08x",&m_outControlBlock[1]);
	
	// initialize the memory buffers
	
	initBuffers();
	PCM_LOG("m_inBuffer[0]=0x%08x",m_inBuffer[0]);
	PCM_LOG("m_inBuffer[1]=0x%08x",m_inBuffer[1]);
	PCM_LOG("m_outBuffer[0]=0x%08x",m_outBuffer[0]);
	PCM_LOG("m_outBuffer[1]=0x%08x",m_outBuffer[1]);
	PCM_LOG("BUS_ADDRESS(0x%08x)=0x%08x",(u32)&m_inControlBlock[0],BUS_ADDRESS((uintptr)&m_inControlBlock[0]));
	

	if (m_initialized)
	{
		PCM_LOG("terminating previous i2s initialization ...",0);
		terminate();
	}

	PCM_LOG("setting up DMA control blocks ...",0);

	if (m_inISR)
	{
		initDMA(false,m_inControlBlock,0,1);
		initDMA(false,m_inControlBlock,1,0);
	}
	if (m_outISR)
	{
		initDMA(true,m_outControlBlock,0,1);
		initDMA(true,m_outControlBlock,1,0);
	}

	// start the BCLK
	
	if (!m_as_slave)
	{
		// Teensy Logic analyzer reads about 1.665 times too slow.
		// Hooking the octo up with rpi as master messes things up.
		//
		// Master TDM (same as on working Raspian Octo):
		//
		//     BCLK = 11.2 MHz (logic 6,812Mhz) = 43750
		//            FCLK = 176khz
		// Master I2S:
		//
		//     BLCK = 2.82 MHz (logic 1.696Mhz) = 44062
		//            FCLK = 44.1khz !! yay
		//
		// SoundInjector Stereo wm8731 master
		//
		//     BCLK = 2.98 Mhz (logic 1.801Mhz) = 46565
		//            FCLK = 46.8khz
		
		assert(CLOCK_FREQ % (m_NUM_CHANNELS * m_CHANNEL_WIDTH) == 0);
		unsigned nDivI = CLOCK_FREQ / (m_NUM_CHANNELS * m_CHANNEL_WIDTH) / m_SAMPLE_RATE;
		unsigned nTemp = CLOCK_FREQ / (m_NUM_CHANNELS * m_CHANNEL_WIDTH) % m_SAMPLE_RATE;
		unsigned nDivF = (nTemp * 4096 + m_SAMPLE_RATE/2) / m_SAMPLE_RATE;
		assert(nDivF <= 4096);
		
		if (nDivF > 4095)
		{
			nDivI++;
			nDivF = 0;
		}

		m_BitClock.Start(nDivI, nDivF, nDivF > 0 ? 1 : 0);
	}
	
	// initialize the I2S peripheral
	// pre-enable DMA
	
	initFrame();
	PeripheralEntry();
	PCM_LOG("pre-enable DMA ...",0);
	
	if (m_inISR)
	{
		write32(ARM_DMA_ENABLE, read32(ARM_DMA_ENABLE) | (1 << m_nDMAInChannel));
		CTimer::SimpleusDelay(1000);
		write32(ARM_DMACHAN_CS(m_nDMAInChannel), CS_RESET);
		while (read32(ARM_DMACHAN_CS(m_nDMAInChannel)) & CS_RESET) {}
	}
	if (m_outISR)
	{
		write32(ARM_DMA_ENABLE, read32(ARM_DMA_ENABLE) | (1 << m_nDMAOutChannel));
		CTimer::SimpleusDelay(1000);
		write32(ARM_DMACHAN_CS(m_nDMAOutChannel), CS_RESET);
		while (read32(ARM_DMACHAN_CS(m_nDMAOutChannel)) & CS_RESET) {}
	}
	
	PeripheralExit();

	PCM_LOG("init() finished",0);
	m_initialized = true;
	
}	// init()



void BCM_PCM::terminate()
{
	PCM_LOG("terminate(%d) ...",m_state);
	
	if (m_state == bcmSoundRunning)
	{
		PCM_LOG("terminate(%d) calling stop()",m_state);
		stop();
		while (m_state == bcmSoundRunning ||
			   m_state == bcmSoundCancelled ||
			   m_state == bcmSoundTerminating );
		PCM_LOG("terminate() state=%d after calling stop()",m_state);
	}
	
	stopPCM();

	if (m_inISR)
	{
		PeripheralEntry();
		write32(ARM_DMACHAN_CS(m_nDMAInChannel), CS_RESET);
		while (read32(ARM_DMACHAN_CS(m_nDMAInChannel)) & CS_RESET) {}
		write32(ARM_DMA_ENABLE, read32(ARM_DMA_ENABLE) & ~(1 << m_nDMAInChannel));
		PeripheralExit();
		
		if (m_bInIRQConnected)
		{
			m_pInterruptSystem->DisconnectIRQ(ARM_IRQ_DMA0+m_nDMAInChannel);
			m_bInIRQConnected = false;
		}
		CMachineInfo::Get()->FreeDMAChannel(m_nDMAInChannel);
		m_nDMAInChannel = 0;
	}
	
	if (m_outISR)
	{
		PeripheralEntry();
		write32(ARM_DMACHAN_CS(m_nDMAOutChannel), CS_RESET);
		while (read32(ARM_DMACHAN_CS(m_nDMAOutChannel)) & CS_RESET) {}
		write32(ARM_DMA_ENABLE, read32(ARM_DMA_ENABLE) & ~(1 << m_nDMAOutChannel));
		PeripheralExit();
		if (m_bOutIRQConnected)
		{
			m_pInterruptSystem->DisconnectIRQ(ARM_IRQ_DMA0+m_nDMAOutChannel);
			m_bOutIRQConnected = false;
		}
		CMachineInfo::Get()->FreeDMAChannel(m_nDMAOutChannel);
		m_nDMAOutChannel = 0;
	}

	m_SAMPLE_RATE = 0;
	m_SAMPLE_SIZE = 0;
	m_CHANNEL_WIDTH = 0;
	m_as_slave = false;
	m_inISR = 0;
	m_outISR = 0;
	m_initialized = false;
	
	PCM_LOG("terminate(%d) finished",m_state);
	
}	// terminate()



void BCM_PCM::initDMA(
	bool output,
	TDMAControlBlock *pb,
	unsigned id,
	unsigned other_id)
{
	// Set the generic Transfer Information bits

	u32 tInfo = 
		(DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
		| TI_WAIT_RESP
		| TI_INTEN;

	// Set output specific Transfer Information bits and pointers to IO

	u8 nBurstLength = 8;
	if (output)
	{
		tInfo |= 0
			| (DREQSourcePCMTX << TI_PERMAP_SHIFT)
			| (nBurstLength << TI_BURST_LENGTH_SHIFT)
			| TI_SRC_WIDTH
			| TI_SRC_INC 
			| TI_DEST_DREQ;
			
		#if TEST_SHORT_CIRCUIT_IN_TO_OUT			
			if (m_inISR)
				pb[id].nSourceAddress = BUS_ADDRESS((uintptr) m_inBuffer[id]);
			else
		#endif
		
		pb[id].nSourceAddress = BUS_ADDRESS((uintptr) m_outBuffer[id]);	
		pb[id].nDestinationAddress = (ARM_PCM_FIFO_A & 0xFFFFFF) + GPU_IO_BASE;
	}
	else	// Set for input (bo is zero)
	{
		tInfo |= 0
			| (DREQSourcePCMRX << TI_PERMAP_SHIFT)
			| (nBurstLength << TI_BURST_LENGTH_SHIFT)
			| TI_DEST_WIDTH
			| TI_DEST_INC
			| TI_SRC_DREQ;	// DMA gated by PCM RX signal
		pb[id].nSourceAddress = (ARM_PCM_FIFO_A & 0xFFFFFF) + GPU_IO_BASE;   // PRH
		pb[id].nDestinationAddress = BUS_ADDRESS((uintptr) m_inBuffer[id]);	
	}

	// finish up
	
	pb[id].nTransferInformation   	= tInfo;
	pb[id].nTransferLength 			= RAW_AUDIO_BLOCK_BYTES;
	pb[id].n2DModeStride       		= 0;
	pb[id].nReserved[0]	       		= 0;
	pb[id].nReserved[1]	       		= 0;
	pb[id].nNextControlBlockAddress = BUS_ADDRESS((uintptr) &pb[other_id]);
	
	CleanAndInvalidateDataCacheRange((uintptr) &pb[id], sizeof (TDMAControlBlock));

}	// initDMA
	


void BCM_PCM::initFrame()
{
	PeripheralEntry();
	PCM_LOG("initFrame() ...",0);
	
	// Write zero which turns everything off
	// and clear the PCM peipheral FIFOs

	write32(ARM_PCM_CS_A, 0);
	CTimer::Get()->usDelay(10);
	write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_TXCLR | CS_A_RXCLR);
	CTimer::Get()->usDelay(10);
	
	// Setup the TXC_A and RXC_A registers (PCM frame definition)
	
	u8 set_extended_bit = 0;
	u32 use_width = m_SAMPLE_SIZE - 8;					// 8 bits are implied
	if (use_width >= 16)								// if larger than 16 
	{
		set_extended_bit = 1;							// set the 1 Channel Width Extension bits
		use_width -= 16;								// and subtract 16
	}
	
	// PRH - NOTE: I HAD TO CHANGE channel 2 width to be
	// (m_CHANNEL_WIDTH+2) << TXC_A_CH2POS__SHIFT for rpi
	// master to wm8731 ... and it appears to be louder
	// with the octo at 0,32 ... so different implementations
	// of i2s devices may have different requirements and..
	// I suspect the same with swapping the L$ clock sense.

	u32 reg_val =
		TXC_A_CH1EN									// Enable channel 1
		| (0 << TXC_A_CH1POS__SHIFT)				// The channel data starts on first (not 0th) BCLK of the frame
		| (set_extended_bit ? TXC_A_CH1WEX : 0) 	// Set the "extended width" bit (if width is 16+)
		| (use_width << TXC_A_CH1WID__SHIFT)		// Set the low 4 channel width bits
		| TXC_A_CH2EN								// Same for channel 2 
		| ((m_CHANNEL_WIDTH + 0) << TXC_A_CH2POS__SHIFT)		// 2 = skip another bit in the 2nd half frame as well
		| (set_extended_bit ? TXC_A_CH2WEX : 0)
		| (use_width << TXC_A_CH2WID__SHIFT);

	if (m_outISR)
		write32(ARM_PCM_TXC_A,reg_val);
	if (m_inISR)
		write32(ARM_PCM_RXC_A,reg_val);
	
	// set the PCM_MODE_A register

	u32 pcm_mode = 0;
	// pcm_mode |= MODE_A_CLKI;			// Invert the BCLK signal sense
		// octo appears to work with either setting of this bit
	
	// FCLK invert
	// had to comment out to get rpi master working with wm3871
	// but it must be set to get correct channel mapping on octo

	pcm_mode |= MODE_A_FSI;				// Frame Sync Invert
	
	
	if (m_as_slave)
	{
		pcm_mode |= MODE_A_CLKM;	// BCLK is an input
		pcm_mode |= MODE_A_FSM;		// FCLK is an input
		pcm_mode |= 1 << MODE_A_FSLEN__SHIFT;
		pcm_mode |= ((2 * m_CHANNEL_WIDTH)-1) << MODE_A_FLEN__SHIFT;
	}

	#if 0
		// When the octo is not hooked up, the following creates a frame
		// that looks exactly like the one coming from the raspian working octo.
		// However, for the octo we set both the codec, and the bcm to slave
		// modes, so this code is not used

		else if (0)	// TDM experiments
		{
			pcm_mode |= MODE_A_FRXP | MODE_A_FTXP;  // frame packed mode              (1 << 24)
			//  pcm_mode |= ((m_NUM_CHANNELS * m_CHANNEL_WIDTH)-1) << MODE_A_FLEN__SHIFT;
			pcm_mode |= ((2 * m_CHANNEL_WIDTH)-1) << MODE_A_FLEN__SHIFT;
			pcm_mode |= m_CHANNEL_WIDTH << MODE_A_FSLEN__SHIFT;
			
		}	
	#endif
	
	else	// master mode, we set the frame params
	{
		pcm_mode |= ((2 * m_CHANNEL_WIDTH)-1) << MODE_A_FLEN__SHIFT;
		// pcm_mode |= ((m_NUM_CHANNELS * m_CHANNEL_WIDTH)-1) << MODE_A_FLEN__SHIFT;
		pcm_mode |= m_CHANNEL_WIDTH << MODE_A_FSLEN__SHIFT;
	}

	write32(ARM_PCM_MODE_A,pcm_mode);

	// other PCM_CS_A register settings
	// "disable standby"
	// enable I2S generally
	// enable RX specifically, and
	// enable TX specifically

	write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_STBY);
	CTimer::Get()->usDelay(50);
	write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_EN);
	CTimer::Get()->usDelay(10);
	
	#if 0 // trying at end of "start()"
		if (m_inISR)
		{
			write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_RXON);
			CTimer::Get()->usDelay(10);
		}
		if (m_outISR)
		{
			write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_TXON);
			CTimer::Get()->usDelay(10);
		}
	#endif
	
	PeripheralExit();
	PCM_LOG("initFrame() finished",0);
	
}


void BCM_PCM::stopPCM(void)
{
	PCM_LOG("stopPCM() ...",0);
	
	PeripheralEntry();
	write32(ARM_PCM_CS_A, 0);
	CTimer::Get()->usDelay(50);
	PeripheralExit();

	m_BitClock.Stop();

	PCM_LOG("stopPCM() finished",0);
}



void BCM_PCM::stop()
{
	PCM_LOG("stop() ...",0);
	if (m_state != bcmSoundRunning)
	{
		PCM_LOG("stop() returning because state=%d\n",m_state);
		return;		
	}

	m_SpinLock.Acquire();
	if (m_state == bcmSoundRunning)
		m_state = bcmSoundCancelled;
	m_SpinLock.Release();

	PCM_LOG("stop() finished",0);

}



//----------------------------------------
// running
//----------------------------------------

void BCM_PCM::start()
{
	if (m_state != bcmSoundIdle)
	{
		PCM_LOG("BCM_PCM::start() returning because state=%d",m_state);
		return;		
	}
	
	PCM_LOG("start() ...",0);
	
	#if INCLUDE_ACTIVITY_LEDS		
		if (m_inISR)
		 	m_RX_ACTIVE.Write(1);
		if (m_outISR)
			m_TX_ACTIVE.Write(1);
	#endif
	
	// re-initialize the memory buffers
	
	initBuffers();		

	// connect IRQs
	
	if (m_inISR && !m_bInIRQConnected)
	{
		PCM_LOG("connecting input IRQ",0);
		m_pInterruptSystem->ConnectIRQ(ARM_IRQ_DMA0+m_nDMAInChannel, audioInIRQStub, this);
		m_bInIRQConnected = true;
	}
	if (m_outISR && !m_bOutIRQConnected)
	{
		PCM_LOG("connecting output IRQ",0);
		m_pInterruptSystem->ConnectIRQ(ARM_IRQ_DMA0+m_nDMAOutChannel, audioOutIRQStub, this);
		m_bOutIRQConnected = true;
	}

	// Initial call to setup DMA buffers
	// These calls give the 0th buffer to the 0th control block
	// for each direction.

	if (m_inISR)
	{
		updateInput(true);
		updateInput(true);
	}
	if (m_outISR)
	{
		updateOutput(true);
		updateOutput(true);
	}

	// enable I2S DMA operation
	
	PeripheralEntry();
	write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_DMAEN);
	PeripheralExit();

	// start DMA
	
	PeripheralEntry();
	if (m_inISR)
	{
		// PCM_LOG("BUS_ADDRESS(0x%08x)=0x%08x",(u32)&m_inControlBlock[0],BUS_ADDRESS((uintptr)&m_inControlBlock[0]));
		
		write32(ARM_DMACHAN_CONBLK_AD(m_nDMAInChannel),
			BUS_ADDRESS((uintptr) &m_inControlBlock[0]));
	
		write32(ARM_DMACHAN_CS(m_nDMAInChannel),
			CS_WAIT_FOR_OUTSTANDING_WRITES
			| (DEFAULT_PANIC_PRIORITY << CS_PANIC_PRIORITY_SHIFT)
			| (DEFAULT_PRIORITY << CS_PRIORITY_SHIFT)
			| CS_ACTIVE);
	}
	
	if (m_outISR)
	{
		write32(ARM_DMACHAN_CONBLK_AD(m_nDMAOutChannel),
			BUS_ADDRESS((uintptr) &m_outControlBlock[0]));
	
		write32(ARM_DMACHAN_CS(m_nDMAOutChannel),
			CS_WAIT_FOR_OUTSTANDING_WRITES
			| (DEFAULT_PANIC_PRIORITY << CS_PANIC_PRIORITY_SHIFT)
			| (DEFAULT_PRIORITY << CS_PRIORITY_SHIFT)
			| CS_ACTIVE);
	}

	PeripheralExit();


	#if BCM_KNOWS_OCTO
		// If the BCM knows about the OCTO, we call
		// setSampleRate() here to start the OCTO clock
		
		// #ifdef SHORT_START
		// 	pControl->shortStart();
		//  #else
		
			pControl->setSampleRate(m_SAMPLE_RATE);
			
		//  #endif
	#endif
	

	if (m_inISR)
	{
		write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_RXON);
		CTimer::Get()->usDelay(10);
	}
	if (m_outISR)
	{
		write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_TXON);
		CTimer::Get()->usDelay(10);
	}

	
	// we are running ..
	
	m_state = bcmSoundRunning;
	
}





//---------------------------------
// Input and Output processing
//---------------------------------


void BCM_PCM::updateInput(bool cold)
	// called twice during setup with cold=1, leaves
	// the toggle at zero.  On the first interrupt
	// buffer[0] will be ready while buffer[1] is being
	// filled. 
{
	// pass the finished ready buffer to client
	// and reset it into the DMA as the next buffer

	CleanAndInvalidateDataCacheRange((uintptr) m_inBuffer[m_inToggle], RAW_AUDIO_BLOCK_BYTES);
	if (!cold)
	{
		#if TEST_SHORT_CIRCUIT_IN_TO_OUT
			// do nothing, input and output buffers are the same
		#elif TEST_ROUTE_IN_TO_OUT
			// copy input to intermediate buffer for output
			memcpy(test_buffer[m_inToggle],m_inBuffer[m_inToggle],RAW_AUDIO_BLOCK_BYTES);
		#else
			#if !OPTIMIZED_IRQS
				assert(m_inISR);
			#endif
			(*m_inISR)();
		#endif
	}
	m_inToggle ^= 1;
}



void BCM_PCM::updateOutput(bool cold)
	// cslled twice during setup, on the first interrupt it will
	// be outputting buffer[1] while we fill in buffer[0]
{
	// get the next client output buffer from the client
	// and give the output buffer to the DMA to output
	
	if (!cold)
	{
		#if OUTPUT_DISTINCTIVE_PATTERN
			memset(m_outBuffer[m_outToggle],0,RAW_AUDIO_BLOCK_BYTES);
			#if 1
				u32 *p = (u32 *) m_outBuffer[m_outToggle];
				for (u32 i=0; i<AUDIO_BLOCK_SAMPLES; i++)
				{
					*p++ = 0x5555;
					*p++ = (i<4) ? 1: 0x7fff;
					p += m_NUM_CHANNELS-2;
				}
			#endif
		#elif TEST_SHORT_CIRCUIT_IN_TO_OUT
			// do nothing, input and output buffers are the same
		#elif TEST_ROUTE_IN_TO_OUT
			// copy the 'ready' input buffer to the pending output buffer
			memcpy(m_outBuffer[m_outToggle],test_buffer[m_inToggle ^ 1],RAW_AUDIO_BLOCK_BYTES);
		#else
			memset(m_outBuffer[m_outToggle],0,RAW_AUDIO_BLOCK_BYTES);
			#if !OPTIMIZED_IRQS
				assert(m_outISR);
			#endif
			(*m_outISR)();
		#endif
	}
	else
	{
		memset(m_outBuffer[m_outToggle],0,RAW_AUDIO_BLOCK_BYTES);
		#if 1
			u32 *p = (u32 *) m_outBuffer[m_outToggle];
			for (u32 i=0; i<AUDIO_BLOCK_SAMPLES; i++)
			{
				*p++ = 0x5555;
				*p++ = (i<4) ? 1: 0x7fff;
				p += m_NUM_CHANNELS-2;
			}
		#endif
	}
	CleanAndInvalidateDataCacheRange((uintptr) m_outBuffer[m_outToggle], RAW_AUDIO_BLOCK_BYTES);
	m_outToggle ^= 1;
}	




//---------------------------------
// IRQ handlers
//---------------------------------

void BCM_PCM::audioInIRQStub(void *pParam)
{
	#if !OPTIMIZED_IRQS
		assert(pParam);
	#endif
	((BCM_PCM *) pParam)->audioInIRQ();
}

void BCM_PCM::audioOutIRQStub(void *pParam)
{
	#if !OPTIMIZED_IRQS
		assert(pParam);
	#endif
	((BCM_PCM *) pParam)->audioOutIRQ();
}


void BCM_PCM::audioInIRQ(void)
{
	PeripheralEntry();
	u32 nIntMask = 1 << m_nDMAInChannel;

	#if !OPTIMIZED_IRQS
		u32 nIntStatus = read32(ARM_DMA_INT_STATUS);
		assert(nIntStatus & nIntMask);
		in_block_count++;
	#endif
	
	write32(ARM_DMA_INT_STATUS, nIntMask);
	u32 nCS = read32(ARM_DMACHAN_CS(m_nDMAInChannel));
	
	#if !OPTIMIZED_IRQS
		assert(nCS & CS_INT);
	#endif
	
	write32(ARM_DMACHAN_CS(m_nDMAInChannel), nCS);	// reset CS_INT
	PeripheralExit();
	
	#if INCLUDE_ACTIVITY_LEDS		
		static u32 rx_count = 0;
		rx_count++;
		if (rx_count > 32)
			// about 10 times a second at 44.1khz with 128 sized buffers
		{
			rx_count = 0;
			m_RX_ACTIVE.Invert();
		}
	#endif

	if (nCS & CS_ERROR)
	{
		m_state = bcmSoundError;
		LOG_ERROR("CS_ERROR in audioInIRQ()",0);
		return;
	}

	m_SpinLock.Acquire();

	switch (m_state)
	{
		case bcmSoundRunning:
			updateInput(false);
			break;
	
		case bcmSoundCancelled:
			PeripheralEntry();
			write32(ARM_DMACHAN_NEXTCONBK(m_nDMAInChannel), 0);
			write32(ARM_DMACHAN_NEXTCONBK(m_nDMAOutChannel), 0);
			PeripheralExit();
			m_state = bcmSoundTerminating;
			break;
	
		case bcmSoundTerminating:
			m_state = bcmSoundIdle;
			break;
	
		default:
			assert(0);
			break;
	}

	m_SpinLock.Release();
}



void BCM_PCM::audioOutIRQ(void)
{
	PeripheralEntry();
	u32 nIntStatus = read32(ARM_DMA_INT_STATUS);
	
	// there is some kind of bug in the rPi or the circle interrupt
	// or DMA handlers that causes THIS routine to be called in
	// either case (or maybe only one interrupt routine is allowed)
	// This little section of code kludgily fixes it and is probably
	// dependent on the order of operations of the IRQ settings ...
	
	#if 1
		u32 nOtherMask = 1 << m_nDMAInChannel;
		if (nIntStatus & nOtherMask)
		{
			#if !OPTIMIZED_IRQS
				wrong_irq_count++;
			#endif
			PeripheralExit();
			audioInIRQ();
			return;
		}
	#endif
	
	// continuing regular code ...
	
	u32 nIntMask = 1 << m_nDMAOutChannel;
	
	#if !OPTIMIZED_IRQS
		assert(nIntStatus & nIntMask);
		out_block_count++;
	#endif
	
	write32(ARM_DMA_INT_STATUS, nIntMask);
	u32 nCS = read32(ARM_DMACHAN_CS(m_nDMAOutChannel));
	#if !OPTIMIZED_IRQS
		assert(nCS & CS_INT);
	#endif
	write32(ARM_DMACHAN_CS(m_nDMAOutChannel), nCS);	// reset CS_INT
	PeripheralExit();
	
	#if INCLUDE_ACTIVITY_LEDS		
		static u32 tx_count = 0;
		tx_count++;
		if (tx_count > 32)		// about 20 times a second
		{
			tx_count = 0;
			m_TX_ACTIVE.Invert();
		}
	#endif
	
	if (nCS & CS_ERROR)
	{
		LOG_ERROR("CS_ERROR in audioOutIRQ()",0);
		m_state = bcmSoundError;
		return;
	}

	m_SpinLock.Acquire();

	switch (m_state)
	{
		case bcmSoundRunning:
			updateOutput(false);
			break;
	
		case bcmSoundCancelled:
			PeripheralEntry();
			write32(ARM_DMACHAN_NEXTCONBK(m_nDMAOutChannel), 0);
			PeripheralExit();
			m_state = bcmSoundTerminating;
			break;
	
		case bcmSoundTerminating:
			m_state = bcmSoundIdle;
			break;
	
		default:
			assert(0);
			break;
	}

	m_SpinLock.Release();
}


