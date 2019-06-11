#ifndef _bcm_pcm_defines_h_
#define _bcm_pcm_defines_h_

#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>

//
// PCM / I2S registers
//
#define CS_A_STBY		        (1 << 25)
#define CS_A_SYNC		        (1 << 24)
#define CS_A_RXSEX              (1 << 23)       // prh added
#define CS_A_RXF                (1 << 22)       // prh added
#define CS_A_TXE		        (1 << 21)
#define CS_A_RXD		        (1 << 20)       // prh added
#define CS_A_TXD		        (1 << 19)
#define CS_A_RXR		        (1 << 18)       // prh added
#define CS_A_TXW		        (1 << 17)
#define CS_A_RXERR	            (1 << 16)       // prh added
#define CS_A_TXERR		        (1 << 15)
#define CS_A_RXSYNC	            (1 << 14)       // prh added (not?)
#define CS_A_TXSYNC		        (1 << 13)
#define CS_A_DMAEN		        (1 << 9)
#define CS_A_RXTHR__SHIFT	    7               // prh added
#define CS_A_TXTHR__SHIFT	    5
#define CS_A_RXCLR		        (1 << 4)
#define CS_A_TXCLR		        (1 << 3)
#define CS_A_TXON		        (1 << 2)
#define CS_A_RXON		        (1 << 1)
#define CS_A_EN			        (1 << 0)
        
#define MODE_A_CLK_DIS          (1 << 28)
#define MODE_A_PDMN             (1 << 27)
#define MODE_A_PDME             (1 << 26)
#define MODE_A_FRXP             (1 << 25)
#define MODE_A_FTXP             (1 << 24)
#define MODE_A_CLKM		        (1 << 23)		// prh added - slave bit clock mode
#define MODE_A_CLKI		        (1 << 22)
#define MODE_A_FSM              (1 << 21)		// prh added - slave frame mode
#define MODE_A_FSI		        (1 << 20)
#define MODE_A_FLEN__SHIFT	    10
#define MODE_A_FSLEN__SHIFT	    0
    
#define TXC_A_CH1WEX	        (1 << 31)
#define TXC_A_CH1EN		        (1 << 30)
#define TXC_A_CH1POS__SHIFT	    20
#define TXC_A_CH1WID__SHIFT	    16
#define TXC_A_CH2WEX	        (1 << 15)
#define TXC_A_CH2EN		        (1 << 14)
#define TXC_A_CH2POS__SHIFT	    4
#define TXC_A_CH2WID__SHIFT	    0

//
// DMA controller
//
#define ARM_DMACHAN_CS(chan)	        (ARM_DMA_BASE + ((chan) * 0x100) + 0x00)
#define CS_RESET			            (1 << 31)
#define CS_ABORT			            (1 << 30)
#define CS_WAIT_FOR_OUTSTANDING_WRITES	(1 << 28)
#define CS_PANIC_PRIORITY_SHIFT		    20
#define DEFAULT_PANIC_PRIORITY		    15
#define CS_PRIORITY_SHIFT		        16
#define DEFAULT_PRIORITY		        1
#define CS_ERROR			            (1 << 8)
#define CS_INT				            (1 << 2)
#define CS_END				            (1 << 1)
#define CS_ACTIVE			            (1 << 0)
    
    
#define ARM_DMACHAN_CONBLK_AD(chan)	    (ARM_DMA_BASE + ((chan) * 0x100) + 0x04)
#define ARM_DMACHAN_TI(chan)		    (ARM_DMA_BASE + ((chan) * 0x100) + 0x08)
#define TI_PERMAP_SHIFT			        16
#define TI_BURST_LENGTH_SHIFT		    12
#define DEFAULT_BURST_LENGTH		    0
#define TI_SRC_IGNORE			        (1 << 11)
#define TI_SRC_DREQ			            (1 << 10)
#define TI_SRC_WIDTH			        (1 << 9)
#define TI_SRC_INC			            (1 << 8)
#define TI_DEST_DREQ			        (1 << 6)
#define TI_DEST_WIDTH			        (1 << 5)
#define TI_DEST_INC			            (1 << 4)
#define TI_WAIT_RESP			        (1 << 3)
#define TI_TDMODE			            (1 << 1)
#define TI_INTEN			            (1 << 0)


#define ARM_DMACHAN_SOURCE_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x0C)
#define ARM_DMACHAN_DEST_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x10)
#define ARM_DMACHAN_TXFR_LEN(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x14)
#define TXFR_LEN_XLENGTH_SHIFT		0
#define TXFR_LEN_YLENGTH_SHIFT		16
#define TXFR_LEN_MAX			    0x3FFFFFFF
#define TXFR_LEN_MAX_LITE		    0xFFFF

#define ARM_DMACHAN_STRIDE(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x18)
#define STRIDE_SRC_SHIFT		    0
#define STRIDE_DEST_SHIFT		    16

#define ARM_DMACHAN_NEXTCONBK(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x1C)
#define ARM_DMACHAN_DEBUG(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x20)
#define DEBUG_LITE			        (1 << 28)

#define ARM_DMA_INT_STATUS		    (ARM_DMA_BASE + 0xFE0)
#define ARM_DMA_ENABLE			    (ARM_DMA_BASE + 0xFF0)


#endif
