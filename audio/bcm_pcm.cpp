
#include "BCM_PCM.h"
#include "bcm_pcm_defines.h"
#include <circle/util.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/alloc.h>
#include <circle/logger.h>
#include <circle/machineinfo.h>

#define log_name "bcm_pcm"

#if 1
	#define PCM_LOG(f,...)           CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
#else
	#define PCM_LOG(...)
#endif




#define PIN_BCLK	   18		// The bit clock
#define PIN_FCLK       19		// The L/R frame clock
#define PIN_RXD		   20       // I2S receive data
#define PIN_TXD		   21		// I2S transmit data
#define PIN_RX_ACTIVE  23		// a LED to show recv activity
#define PIN_TX_ACTIVE  24		// a LED to show xmit activity


#define CLOCK_FREQ		500000000
	// This is probably wrong.
	// #define CLOCK_FREQ  	303408000	
#define NUM_HW_CHANNELS  2
	// THIS IS ALWAYS TWO (2) FOR I2S


// static instance

BCM_PCM bcm_pcm;


//--------------------------------
// ctor
//--------------------------------

BCM_PCM::BCM_PCM() :
	m_pInterruptSystem(0),
	m_BCLK(PIN_BCLK, GPIOModeAlternateFunction0),
	m_FCLK(PIN_FCLK, GPIOModeAlternateFunction0),
	m_RXD(PIN_RXD, 	 GPIOModeAlternateFunction0),
	m_TXD(PIN_TXD, 	 GPIOModeAlternateFunction0),
	m_BitClock(GPIOClockPCM, GPIOClockSourcePLLD),
		// GPIOClockSourceOscillator = 1,		// 19.2 MHz
		// GPIOClockSourcePLLC       = 5,		// 1000 MHz (changes with overclock settings)
		// GPIOClockSourcePLLD       = 6,		// 500 MHz hence the #define CLOCK_FREQ 500000000
		// GPIOClockSourceHDMI       = 7		// 216 MHz			
	m_RX_ACTIVE(PIN_RX_ACTIVE, GPIOModeOutput),
	m_TX_ACTIVE(PIN_TX_ACTIVE, GPIOModeOutput),
	m_nDMAInChannel(CMachineInfo::Get()->AllocateDMAChannel(DMA_CHANNEL_LITE)),
	m_nDMAOutChannel(CMachineInfo::Get()->AllocateDMAChannel(DMA_CHANNEL_LITE))
{
	m_SAMPLE_RATE = 0;
	m_SAMPLE_SIZE = 0;
	m_CHANNEL_LENGTH = 0;
	m_as_slave = false;
	
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
	
	initBuffers();
	
	m_RX_ACTIVE.Write(1);
	m_TX_ACTIVE.Write(1);
}
	
	
BCM_PCM::~BCM_PCM()
{
	if (m_initialized)
		terminate();
	m_pInterruptSystem = 0;
}


void BCM_PCM::initBuffers()
{
	m_inToggle = 0;
	m_outToggle = 0;
	for (int i=0; i<2; i++)
	{
		memset(m_inBuffer[i],0,RAW_AUDIO_BLOCK_BYTES);
		memset(m_outBuffer[i],0,RAW_AUDIO_BLOCK_BYTES);
	}
	
	in_block_count 	= 0;
	out_block_count = 0;
	underflow_count = 0;
	overflow_count 	= 0;
}


//-----------------------------
// initialization
//-----------------------------

	
void BCM_PCM::init(
		u32   	  sample_rate,
		u8    	  sample_size,
		u8    	  channel_length,
		bool  	  as_slave)
{
	assert(m_inISR || m_outISR);

	if (m_state >= bcmSoundRunning)
	{
		PCM_LOG("BCM_PCM::init(%d) returning because state=%d",as_slave,m_state);
		return;		
	}

	m_RX_ACTIVE.Write(0);
	m_TX_ACTIVE.Write(0);
		
	PCM_LOG("init(%d,%d,%d,%d,%s) ...",
		sample_rate,
		sample_size,
		channel_length,
		as_slave,
		((u32)m_inISR) & ((u32)m_outISR) ? "BOTH" :
		((u32)m_inISR) ? "INPUT" :
		((u32)m_outISR) ? "OUTPUT" :
		"ERROR - no isr specified in init!");
	
	PCM_LOG("inDMAChannel=%d outDMAChannel=%d",
		m_nDMAInChannel,
		m_nDMAOutChannel);
	
	m_pInterruptSystem = CInterruptSystem::Get();
	assert(m_pInterruptSystem);

	if (m_initialized)
	{
		PCM_LOG("terminating previous i2s initialization ...",0);
		terminate();
	}

	m_SAMPLE_RATE = sample_rate;
	m_SAMPLE_SIZE = sample_size;
	m_CHANNEL_LENGTH = channel_length;
	m_as_slave = as_slave;		
	
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
		PCM_LOG("starting BCLK ...",0);
		assert(CLOCK_FREQ % (NUM_HW_CHANNELS * NUM_HW_CHANNELS) == 0);
		unsigned nDivI = CLOCK_FREQ / (NUM_HW_CHANNELS * m_CHANNEL_LENGTH) / m_SAMPLE_RATE;
		unsigned nTemp = CLOCK_FREQ / (NUM_HW_CHANNELS * m_CHANNEL_LENGTH) % m_SAMPLE_RATE;
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
	
	initI2S();
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
	
	stopI2S();

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
	m_CHANNEL_LENGTH = 0;
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

	if (output)
	{
		tInfo |=
			(DREQSourcePCMTX << TI_PERMAP_SHIFT)
			| TI_SRC_WIDTH
			| TI_SRC_INC 
			| TI_DEST_DREQ;
		pb[id].nDestinationAddress = (ARM_PCM_FIFO_A & 0xFFFFFF) + GPU_IO_BASE;
	}
	else	// Set for input (bo is zero)
	{
		tInfo |=
			(DREQSourcePCMRX << TI_PERMAP_SHIFT)
			| TI_DEST_WIDTH
			| TI_DEST_INC
			| TI_SRC_DREQ;	// DMA gated by PCM RX signal
		pb[id].nSourceAddress = (ARM_PCM_FIFO_A & 0xFFFFFF) + GPU_IO_BASE;   // PRH
	}

	// finish up
	
	pb[id].nTransferInformation   	= tInfo;
	pb[id].n2DModeStride       		= 0;
	pb[id].nReserved[0]	       		= 0;
	pb[id].nReserved[1]	       		= 0;
	pb[id].nNextControlBlockAddress = BUS_ADDRESS((uintptr) &pb[other_id]);

}	// initDMA
	


void BCM_PCM::initI2S()
{
	PeripheralEntry();
	PCM_LOG("initI2S() ...",0);
	
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

	u32 reg_val =
		TXC_A_CH1EN									// Enable channel 1
		| (1 << TXC_A_CH1POS__SHIFT)				// The channel data starts on first (not 0th) BCLK of the frame
		| (set_extended_bit ? TXC_A_CH1WEX : 0) 	// Set the "extended width" bit (if width is 16+)
		| (use_width << TXC_A_CH1WID__SHIFT)		// Set the low 4 channel width bits
		| TXC_A_CH2EN								// Same for channel 2 
		| ((m_CHANNEL_LENGTH+1) << TXC_A_CH2POS__SHIFT)
		| (set_extended_bit ? TXC_A_CH2WEX : 0)
		| (use_width << TXC_A_CH2WID__SHIFT);

	if (m_outISR)
		write32(ARM_PCM_TXC_A,reg_val);
	if (m_inISR)
		write32(ARM_PCM_RXC_A,reg_val);
	
	// set the PCM_MODE_A register

	u32 pcm_mode = 0;
	pcm_mode |= MODE_A_CLKI;			// Invert the BCLK signal sense
	pcm_mode |= MODE_A_FSI;				// Frame Sync Invert
	if (m_as_slave)
	{
		pcm_mode |= MODE_A_CLKM;	// BCLK is an input
		pcm_mode |= MODE_A_FSM;		// FCLK is an input
	}
	else
	{
		pcm_mode |= ((NUM_HW_CHANNELS * m_CHANNEL_LENGTH)-1) << MODE_A_FLEN__SHIFT;
		pcm_mode |= m_CHANNEL_LENGTH << MODE_A_FSLEN__SHIFT;
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

	PeripheralExit();
	PCM_LOG("initI2S() finished",0);
	
}


void BCM_PCM::stopI2S(void)
{
	PCM_LOG("stopI2S() ...",0);
	
	PeripheralEntry();
	write32(ARM_PCM_CS_A, 0);
	CTimer::Get()->usDelay(50);
	PeripheralExit();
	m_BitClock.Stop();

	PCM_LOG("stopI2S() finished",0);
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
// same old shit ... I'm not getting any interrupts


void BCM_PCM::start()
{

	if (m_state != bcmSoundIdle)
	{
		PCM_LOG("BCM_PCM::start() returning because state=%d",m_state);
		return;		
	}
	
	PCM_LOG("start() ...",0);
	if (m_inISR)
		m_RX_ACTIVE.Write(1);
	if (m_outISR)
		m_TX_ACTIVE.Write(1);

	// we are running ..

	m_state = bcmSoundRunning;

	// re-initialize the memory buffers
	
	initBuffers();		

	// Initial call to setup DMA buffers
	// These calls give the 0th buffer to the 0th control block
	// for each direction.
	
	if (m_inISR)	
		updateInput(true);
	if (m_outISR)
		updateOutput(true);

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

	// enable I2S DMA operation
	
	PeripheralEntry();
	write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_DMAEN);

	// start DMA
	
	if (m_inISR)
	{
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
	
	// At this point we have set the 0th INPUT and OUTPUT
	// control blocks, which have pointers to the 0th
	// input and output buffers.  The processor should
	// happily be dma'ing the output buffer to the codec
	// fifo and receiving the input buffer from the fifo.
	//	
	// By the time we get the first interrupt, remember
	// that the 2nd DMAs will already be underway. We have
	// to keep one step ahead in supplying buffers!
	
	// So, here we call the "update" methods again. These
	// calls will set the control_block[1]'s with pointers
	// to buffer[1]'s and after these calls, the toggle bits
	// will be reset back to zero.
	//
	// So, when the first interrupt happens, and the DMA
	// starts working with control_block and buffer #1,
	// we will pass buffers #0 to the client, where they
	// can use the data in the input buffer, and/or fill
	// fill in for the subsequent interrupt(s)
	
	if (m_inISR)
		updateInput(true);	
	if (m_outISR)
		updateOutput(true);

		
	// PCM_LOG("start() finished",0);
	// m_RX_ACTIVE.Write(0);
	// m_TX_ACTIVE.Write(0);
}





//---------------------------------
// Input and Output processing
//---------------------------------


void BCM_PCM::updateInput(bool cold)
	// this method is called twice during initialization with cold=1
	// to setup the first two input buffers.  Thereafter, it is called
	// with cold=0 by the irq handler, signifying that the buffer indicated
	// by m_inToggle is ready for the client to process. They must process
	// (copy) it to another buffer, and return in time for us to pass it
	// back to the DMA as the next buffer ...
{
	// pass the finished ready buffer to client

	if (!cold)
	{
		CleanAndInvalidateDataCacheRange((uintptr)  m_inBuffer[ m_inToggle ], RAW_AUDIO_BLOCK_BYTES);
		assert(m_inISR);
		(*m_inISR)();
	}

	// set it into the DMA as the next buffer
	
	uint32_t *buf = m_inBuffer[ m_inToggle ];
	m_inControlBlock[m_inToggle].nTransferLength = RAW_AUDIO_BLOCK_BYTES;
	m_inControlBlock[m_inToggle].nDestinationAddress = BUS_ADDRESS((uintptr)buf);
	CleanAndInvalidateDataCacheRange((uintptr) buf, RAW_AUDIO_BLOCK_BYTES);
	CleanAndInvalidateDataCacheRange((uintptr) &m_inControlBlock[m_inToggle], sizeof (TDMAControlBlock));
	m_inToggle ^= 1;

}	// updateInput()



void BCM_PCM::updateOutput(bool cold)
	// The first two cold calls pass two zeroed bufs to the output DMA.
	// Subsequently we will call the client isr method and allow them
	// to fill in the buffers as they choose.  If the client isr is
	// called, then they MUST call toggleOutput() when the buffer is
	// ready to be handed to the dma, otherwise, we call it here
{
	// get the next client output buffer from the client
	
	if (!cold)
	{
		CleanAndInvalidateDataCacheRange((uintptr)  m_outBuffer[ m_outToggle ], RAW_AUDIO_BLOCK_BYTES);
		assert(m_outISR);
		(*m_outISR)();
	}

	// give the output buffer to the DMA to output
	
	uint32_t *buf = m_outBuffer[m_outToggle];
	m_outControlBlock[m_outToggle].nTransferLength = RAW_AUDIO_BLOCK_BYTES;
	m_outControlBlock[m_outToggle].nSourceAddress = BUS_ADDRESS((uintptr) buf);
	CleanAndInvalidateDataCacheRange((uintptr) buf, RAW_AUDIO_BLOCK_BYTES);
	CleanAndInvalidateDataCacheRange((uintptr) &m_outControlBlock[m_outToggle], sizeof (TDMAControlBlock));
	m_outToggle ^= 1;
	
}	// updateOutput()




//---------------------------------
// IRQ handlers
//---------------------------------

void BCM_PCM::audioInIRQStub(void *pParam)
{
	BCM_PCM *p_this = (BCM_PCM *) pParam;
	assert(p_this != 0);
	p_this->audioInIRQ();
}

void BCM_PCM::audioOutIRQStub(void *pParam)
{
	BCM_PCM *p_this = (BCM_PCM *) pParam;
	assert(p_this != 0);
	p_this->audioOutIRQ();
}


void BCM_PCM::audioInIRQ(void)
{
	printf("inIRQ!!\n");
	if (m_state != bcmSoundRunning)
		return;
	
	PeripheralEntry();
	u32 nIntStatus = read32(ARM_DMA_INT_STATUS);
	u32 nIntMask = 1 << m_nDMAInChannel;
	
	assert(nIntStatus & nIntMask);
	in_block_count++;
	
	write32(ARM_DMA_INT_STATUS, nIntMask);
	u32 nCS = read32(ARM_DMACHAN_CS(m_nDMAInChannel));
	assert(nCS & CS_INT);
	write32(ARM_DMACHAN_CS(m_nDMAInChannel), nCS);	// reset CS_INT
	PeripheralExit();
	
	static u32 rx_count = 0;
	rx_count++;
	if (rx_count > 16)		// about 20 times a second
	{
		rx_count = 0;
		m_RX_ACTIVE.Invert();
	}

	if (nCS & CS_ERROR)
	{
		m_state = bcmSoundError;
		printf("CS_ERROR in audioInIRQ()");
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
	printf("outIRQ!!\n");
	if (m_state != bcmSoundRunning)
		return;

	PeripheralEntry();
	assert(m_state != bcmSoundIdle);

	u32 nIntStatus = read32(ARM_DMA_INT_STATUS);
	u32 nIntMask = 1 << m_nDMAOutChannel;
	
	// there is some kind of bug in the rPi or the circle interrupt
	// or DMA handlers that causes THIS routine to be called in
	// either case (or maybe only one interrupt routine is allowed)
	// This little section of code kludgily fixes it and is probably
	// dependent on the order of operations of the IRQ settings ...
	
	#if 1
		u32 nOtherMask = 1 << m_nDMAInChannel;
		if (nIntStatus & nOtherMask)
		{
			PeripheralExit();
			audioInIRQ();
			return;
		}
	#endif
	
	out_block_count++;
	
	// continuing regular code ...
	
	assert(nIntStatus & nIntMask);
	write32(ARM_DMA_INT_STATUS, nIntMask);
	u32 nCS = read32(ARM_DMACHAN_CS(m_nDMAOutChannel));
	assert(nCS & CS_INT);
	write32(ARM_DMACHAN_CS(m_nDMAOutChannel), nCS);	// reset CS_INT
	PeripheralExit();
	
	static u32 tx_count = 0;
	tx_count++;
	if (tx_count > 16)		// about 20 times a second
	{
		tx_count = 0;
		m_TX_ACTIVE.Invert();
	}

	if (nCS & CS_ERROR)
	{
		printf("CS_ERROR in audioOutIRQ()");
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


