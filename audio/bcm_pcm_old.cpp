// it requires three buffers to manage the ping-pong dma
//
// for example, on input:
//
// it is first setup with two buffers.  Then on the 1st interrupt:
//
//     buffer[0]   is input available to the user
//     buffer[1]   is currently being filled by the DMA
//     buffer[2]   is the one we are setting as the "next" buffer
//
// The teensy audio library wants to use two buffers where
// on the 1st interrupt
//
//     buffer[0] is available
//     buffer[1] is being filled
//
// that means that the audio library must empty buffer[0] very quickly
// and then let us assign it as the next DMA buffer, well before [1] is
// filled.



// interrupt, the 0th buffer is available to the user, 
// while 1st buffer is already being filled and we simultanously
// set the 2nd buffer into 2nd

#include "bcm_pcm.h"
#include "bcm_pcm_defines.h"

#include <circle/interrupt.h>
#include <circle/dmachannel.h>
#include <circle/spinlock.h>
#include <circle/types.h>
#include <circle/machineinfo.h>
#include <circle/logger.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/alloc.h>
#include <circle/util.h>
#include <assert.h>

#define log_name "bcm_pcm"


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


BCM_PCM bcm_pcm;


//--------------------------------
// ctor
//--------------------------------

BCM_PCM::BCM_PCM() :
	m_BCLK(PIN_BCLK, GPIOModeAlternateFunction0),
	m_FCLK(PIN_FCLK, GPIOModeAlternateFunction0),
	m_RXD(PIN_RXD, 	 GPIOModeAlternateFunction0),
	m_TXD(PIN_TXD, 	 GPIOModeAlternateFunction0),
	m_BitClock(GPIOClockPCM, GPIOClockSourcePLLD),
	#if WITH_ACTIVITY_LEDS
		m_rxActive(PIN_RX_ACTIVE, GPIOModeOutput),
		m_txActive(PIN_TX_ACTIVE, GPIOModeOutput),		
	#endif
	m_nDMAInChannel(CMachineInfo::Get()->AllocateDMAChannel(DMA_CHANNEL_LITE)),
	m_nDMAOutChannel(CMachineInfo::Get()->AllocateDMAChannel(DMA_CHANNEL_LITE))
{
	m_SAMPLE_RATE = 0;
	m_SAMPLE_SIZE = 0;
	m_CHANNEL_LENGTH = 0;
	m_as_slave = false;
	m_direction = 0;

	m_initialized = false;
	m_pInterruptSystem = 0;
	m_bInIRQConnected = 0;
	m_bOutIRQConnected = 0;
	
	m_dma_len = 0;
	
	m_inToggle = 0;
	m_outToggle = 0;
	m_inBuffer[0] = 0;
	m_inBuffer[1] = 0;
	m_outBuffer[0] = 0;
	m_outBuffer[1] = 0;
}
	
	
BCM_PCM::~BCM_PCM()
{
	// if (m_initialized)
	// 	terminate();
	m_pInterruptSystem = 0;
}

uint32_t *BCM_PCM::getInBuffer()
{
	#if WITH_ACTIVITY_LEDS
		m_rxActive.Write(1);
	#endif
	return m_inBuffer[m_inToggle];
}

uint32_t *BCM_PCM::getOutBuffer()
{
	#if WITH_ACTIVITY_LEDS
		m_txActive.Write(1);
	#endif
	return m_outBuffer[m_outToggle];
}



//-----------------------------
// initialization
//-----------------------------
	
void BCM_PCM::init(
		u32   sample_rate,
		u8    sample_size,
		u8    channel_length,
		bool  as_slave,
		u8    direction /* = AUDIO_IN | AUDIO_OUT */)
{
	LOG("init(%d,%d,%d,%d,%d)",sample_rate,sample_size,channel_length,as_slave,direction);

	if (m_initialized)
	{
		LOG_WARNING("BCM_PCM is already initialized!",0);
		return;
	}
	
	m_pInterruptSystem = CInterruptSystem::Get();
	assert(m_pInterruptSystem);

	m_SAMPLE_RATE = sample_rate;
	m_SAMPLE_SIZE = sample_size;
	m_CHANNEL_LENGTH = channel_length;
	m_as_slave = as_slave;		
	m_direction = direction;
	
	LOG("setup DMA control blocks ...",0);

	if (m_direction & AUDIO_IN)
	{
		initDMA(false,0);
		initDMA(false,1);
		m_inControlBlock[0].nNextControlBlockAddress = BUS_ADDRESS((uintptr) &m_inControlBlock[1]);
		m_inControlBlock[1].nNextControlBlockAddress = BUS_ADDRESS((uintptr) &m_inControlBlock[0]);
	}
	if (m_direction & AUDIO_OUT)
	{
		initDMA(true,0);
		initDMA(true,1);
		m_outControlBlock[0].nNextControlBlockAddress = BUS_ADDRESS((uintptr) &m_outControlBlock[1]);
		m_outControlBlock[1].nNextControlBlockAddress = BUS_ADDRESS((uintptr) &m_outControlBlock[0]);
	}	

	// start the BCLK
	
	if (!m_as_slave)
	{
		LOG("starting BCLK",0);
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
	
	initI2s();
	
	PeripheralEntry();
	LOG("pre-enable DMA ...",0);
	
	if (m_direction & AUDIO_IN)
	{
		write32(ARM_DMA_ENABLE, read32(ARM_DMA_ENABLE) | (1 << m_nDMAInChannel));
		CTimer::SimpleusDelay(1000);
		write32(ARM_DMACHAN_CS(m_nDMAInChannel), CS_RESET);
		while (read32(ARM_DMACHAN_CS(m_nDMAInChannel)) & CS_RESET) {}
	}
	if (m_direction & AUDIO_OUT)
	{
		write32(ARM_DMA_ENABLE, read32(ARM_DMA_ENABLE) | (1 << m_nDMAOutChannel));
		CTimer::SimpleusDelay(1000);
		write32(ARM_DMACHAN_CS(m_nDMAOutChannel), CS_RESET);
		while (read32(ARM_DMACHAN_CS(m_nDMAOutChannel)) & CS_RESET) {}
	}
	PeripheralExit();

	m_initialized = true;
	LOG("init() finished",0);
}





void BCM_PCM::initI2s()
{
	PeripheralEntry();
	LOG("initI2S()",0);
	
	// Write zero which turns everything off
	// and clear the PCM peipheral FIFOs

	write32(ARM_PCM_CS_A, 0);
	CTimer::Get()->usDelay(10);
	write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_TXCLR | CS_A_RXCLR);
	CTimer::Get()->usDelay(10);
	
	//--------------------------------------------------------------
	// Setup the TXC_A and RXC_A registers (PCM frame definition)
	//--------------------------------------------------------------
	
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

	if (m_direction & AUDIO_OUT)
		write32(ARM_PCM_TXC_A,reg_val);
	if (m_direction & AUDIO_IN)							// Setup RXC_A regsiter
		write32(ARM_PCM_RXC_A,reg_val);
	
	//--------------------------------------
	// set the PCM_MODE_A register
	//--------------------------------------

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

	//------------------------------------------
	// other PCM_CS_A register settings
	//------------------------------------------
	// "disable standby"
	// enable I2S generally
	// enable RX specifically, and
	// enable TX specifically

	write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_STBY);
	CTimer::Get()->usDelay(50);
	write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_EN);
	CTimer::Get()->usDelay(10);
	if (m_direction & AUDIO_IN)
	{
		write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_RXON);
		CTimer::Get()->usDelay(10);
	}
	if (m_direction & AUDIO_OUT)
	{
		write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_TXON);
		CTimer::Get()->usDelay(10);
	}

	PeripheralExit();
	LOG("initI2S() finished",0);
}



void BCM_PCM::initDMA(bool output, unsigned id)
{
	LOG("initDMA(%d,%d)",output,id);
	
	TDMAControlBlock *pb = output ?
		&m_outControlBlock[id] :
		&m_inControlBlock[id];

	// Set the generic Transfer Information bits
	
	u32 tInfo = 
		(DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
		| TI_WAIT_RESP
		| TI_INTEN;

	// Set output specific Transfer Information bits and pointer

	if (output)
	{
		tInfo |=
			(DREQSourcePCMTX << TI_PERMAP_SHIFT)
			| TI_SRC_WIDTH
			| TI_SRC_INC 
			| TI_DEST_DREQ;
		pb->nDestinationAddress = (ARM_PCM_FIFO_A & 0xFFFFFF) + GPU_IO_BASE;
	}
	else	// Set for input 
	{
		tInfo |=
			(DREQSourcePCMRX << TI_PERMAP_SHIFT)
			| TI_DEST_WIDTH
			| TI_DEST_INC
			| TI_SRC_DREQ;	// DMA gated by PCM RX signal
		pb->nSourceAddress = (ARM_PCM_FIFO_A & 0xFFFFFF) + GPU_IO_BASE;  
	}

	// set the transfer information and standard members
	
	pb->nTransferInformation    	= tInfo;
	pb->n2DModeStride       		= 0;
	pb->nReserved[0]	       		= 0;
	pb->nReserved[1]	       		= 0;

	LOG("initDMA(%d,%d) finished",output,id);

}	// initDMA()
	


void my_isr(void *param)
{
	printf("isr \n");
}


//-----------------------------
// running
//-----------------------------

void BCM_PCM::begin(
	bool   		output,
	isr_method 	*isr,
	u16			len,
	uint32_t	*buf0,
	uint32_t 	*buf1)
{
	LOG("begin(%d,%d)",output,len);
	assert(len);
	assert(buf0);
	assert(buf1);
	
	m_dma_len = len * 4;
	memset(buf0,0,m_dma_len);
	memset(buf1,0,m_dma_len);
	
	typedef void circle_isr_method(void *pThis);
	circle_isr_method *fxn_ptr = (circle_isr_method *) isr;
		// cast the regular static isr method to a circle signature
		// which includes the void *pthis memver

	if (output)
	{
		if (!m_bOutIRQConnected)
		{
			printf("connecting Out interrupt to dma channel %d\n",m_nDMAOutChannel);
			m_pInterruptSystem->ConnectIRQ(ARM_IRQ_DMA0+m_nDMAOutChannel, fxn_ptr, 0);
			m_bOutIRQConnected = true;
		}

		m_outBuffer[0] = buf0;
		m_outBuffer[1] = buf1;
		m_outControlBlock[0].nTransferLength = m_dma_len;		
		m_outControlBlock[0].nSourceAddress = BUS_ADDRESS((uintptr) m_outBuffer[0]);
		CleanAndInvalidateDataCacheRange((uintptr) m_outBuffer[0], m_dma_len);
		CleanAndInvalidateDataCacheRange((uintptr) &m_outControlBlock[0], sizeof (TDMAControlBlock));

		PeripheralEntry();
		write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_DMAEN);
		
		write32(ARM_DMACHAN_CONBLK_AD(m_nDMAOutChannel),
			BUS_ADDRESS((uintptr) &m_outControlBlock[0]));
	
		write32(ARM_DMACHAN_CS(m_nDMAOutChannel),
			CS_WAIT_FOR_OUTSTANDING_WRITES
			| (DEFAULT_PANIC_PRIORITY << CS_PANIC_PRIORITY_SHIFT)
			| (DEFAULT_PRIORITY << CS_PRIORITY_SHIFT)
			| CS_ACTIVE);

		PeripheralExit();

	}
	else
	{
		if (!m_bInIRQConnected)
		{
			printf("connecting In interrupt to dma channel %d\n",m_nDMAInChannel);
			m_pInterruptSystem->ConnectIRQ(ARM_IRQ_DMA0+m_nDMAInChannel, fxn_ptr, 0);
			m_bInIRQConnected = true;
		}

		m_inBuffer[0] = buf0;
		m_inBuffer[1] = buf1;
		m_inControlBlock[0].nTransferLength = m_dma_len;		
		m_inControlBlock[0].nDestinationAddress = BUS_ADDRESS((uintptr) m_inBuffer[0]);		
		CleanAndInvalidateDataCacheRange((uintptr) m_inBuffer[0], m_dma_len);
		CleanAndInvalidateDataCacheRange((uintptr) &m_inControlBlock[0], sizeof (TDMAControlBlock));

		PeripheralEntry();
		write32(ARM_PCM_CS_A, read32(ARM_PCM_CS_A) | CS_A_DMAEN);
		
		write32(ARM_DMACHAN_CONBLK_AD(m_nDMAInChannel),
			BUS_ADDRESS((uintptr) &m_inControlBlock[0]));
	
		write32(ARM_DMACHAN_CS(m_nDMAInChannel),
			CS_WAIT_FOR_OUTSTANDING_WRITES
			| (DEFAULT_PANIC_PRIORITY << CS_PANIC_PRIORITY_SHIFT)
			| (DEFAULT_PRIORITY << CS_PRIORITY_SHIFT)
			| CS_ACTIVE);

		PeripheralExit();
	}
	
	LOG("begin(%d,%d) finished",output,len);
}



void BCM_PCM::toggleIn()
{
	m_inToggle ^= 1;
	m_inControlBlock[m_inToggle].nDestinationAddress = BUS_ADDRESS((uintptr) m_inBuffer[m_inToggle]);
	m_inControlBlock[m_inToggle].nTransferLength = m_dma_len;
	CleanAndInvalidateDataCacheRange((uintptr) m_inBuffer[m_inToggle], m_dma_len);
	CleanAndInvalidateDataCacheRange((uintptr) &m_inControlBlock[m_inToggle], sizeof (TDMAControlBlock));
	
	#if WITH_ACTIVITY_LEDS
		m_rxActive.Write(0);
	#endif
	
}



void BCM_PCM::toggleOut()
{
	m_outToggle ^= 1;
	m_inControlBlock[m_outToggle].nDestinationAddress = BUS_ADDRESS((uintptr) m_outBuffer[m_inToggle]);
	m_inControlBlock[m_outToggle].nTransferLength = m_dma_len;
	CleanAndInvalidateDataCacheRange((uintptr) m_outBuffer[m_outToggle], m_dma_len);
	CleanAndInvalidateDataCacheRange((uintptr) &m_outControlBlock[m_outToggle], sizeof (TDMAControlBlock));
	
	#if WITH_ACTIVITY_LEDS
		m_txActive.Write(0);
	#endif
}



void BCM_PCM::clearIRQ(bool output)
{
	LOG("clearIRQ(%d)",output);
	unsigned nDMAChannel = output ? m_nDMAOutChannel : m_nDMAInChannel;
	
	PeripheralEntry();
	u32 nIntStatus = read32(ARM_DMA_INT_STATUS);
	u32 nIntMask = 1 << nDMAChannel;
	assert(nIntStatus & nIntMask);
	write32(ARM_DMA_INT_STATUS, nIntMask);
	u32 nCS = read32(ARM_DMACHAN_CS(nDMAChannel));
	assert(nCS & CS_INT);
	write32(ARM_DMACHAN_CS(nDMAChannel), nCS);	
	PeripheralExit();
}

