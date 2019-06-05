/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <Arduino.h>
#include "input_i2s.h"
#include "output_i2s.h"
#ifdef __circle__

	#include "bcm_pcm.h"
	#include <circle/logger.h>
	#define log_name "input_i2s"
	#if 0
		#define I2I_LOG(f,...)           CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
	#else
		#define I2I_LOG(...)
	#endif
	
#else
	#define I2I_LOG(...)
	DMAChannel AudioInputI2S::dma(false);
	DMAMEM static uint32_t i2s_rx_buffer[AUDIO_BLOCK_SAMPLES];
#endif


audio_block_t * AudioInputI2S::block_left = NULL;
audio_block_t * AudioInputI2S::block_right = NULL;
uint16_t AudioInputI2S::block_offset = 0;
bool AudioInputI2S::update_responsibility = false;


void AudioInputI2S::begin(void)
{
	#ifdef __circle__

		I2I_LOG("begin()",0);
		AudioOutputI2S::config_i2s();
		update_responsibility = update_setup();
		
		// allocate the left and right user blocks
		// 
		// AudioInputI2S::block_left = allocate();
		// AudioInputI2S::block_right = allocate();
		// assert(AudioInputI2S::block_left);
		// assert(AudioInputI2S::block_right);
		
		bcm_pcm.start();
		
		I2I_LOG("begin() finished",0);

	#else	// !__cirle__
	
		dma.begin(true); // Allocate the DMA channel first
	
		//block_left_1st = NULL;
		//block_right_1st = NULL;
	
		// TODO: should we set & clear the I2S_RCSR_SR bit here?
		AudioOutputI2S::config_i2s();
	
		CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0
		#if defined(KINETISK)
			dma.TCD->SADDR = (void *)((uint32_t)&I2S0_RDR0 + 2);
			dma.TCD->SOFF = 0;
			dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
			dma.TCD->NBYTES_MLNO = 2;
			dma.TCD->SLAST = 0;
			dma.TCD->DADDR = i2s_rx_buffer;
			dma.TCD->DOFF = 2;
			dma.TCD->CITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;
			dma.TCD->DLASTSGA = -sizeof(i2s_rx_buffer);
			dma.TCD->BITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;
			dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
		#endif
		dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);
		update_responsibility = update_setup();
		dma.enable();
	
		I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
		I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE; // TX clock enable, because sync'd to TX
		dma.attachInterrupt(isr);
		
	#endif	// !__circle__
}


void AudioInputI2S::isr(void)
{
	#ifdef __circle__
		
		// get the uint32 'ready' input dma buffer from the bcm_pcm
		
		u16 len = AUDIO_BLOCK_SAMPLES;
		uint32_t *src = bcm_pcm.getInBuffer();
		
		// we always do a full buffer at a time, so
		// block_offset is always AUDIO_BLOCK_SAMPLES
		
		block_offset = AUDIO_BLOCK_SAMPLES;
		
		// get pointers to the destination (client) int16's and loop
		// moving pairs of u32's from the dma buffer into pairs
		// of int16's in the "client" teensy audio blocks

		audio_block_t *left = AudioInputI2S::block_left;
		audio_block_t *right = AudioInputI2S::block_right;
		int16_t *dest_left = left ? left->data : 0;
		int16_t *dest_right = right ? right->data : 0;
		
		if (left && right)
		{
			while (len--)
			{
				*dest_left++ = *(int16_t *) *src++;
				*dest_right++ = *(int16_t *) *src++;
			}
		}
		else if (left)
		{
			while (len--)
			{
				*dest_left++ = *(int16_t *) *src++;
				src++;
			}
		}
		else if (right)
		{
			while (len--)
			{
				src++;
				*dest_right++ = *(int16_t *) *src++;
			}
		}
		
		// buffer is ready: call the client update method.
		
		if (AudioInputI2S::update_responsibility)
			AudioStream::update_all();
			
		// this routine MUST complete before the DMA issues
		// the next interrupt!! 
	
	#else

		uint32_t daddr, offset;
		int16_t *dest_left, *dest_right;
		audio_block_t *left, *right;
	
		//digitalWriteFast(3, HIGH);
		#if defined(KINETISK)
			daddr = (uint32_t)(dma.TCD->DADDR);
		#endif
			dma.clearInterrupt();

		if (daddr < (uint32_t)i2s_rx_buffer + sizeof(i2s_rx_buffer) / 2)
		{
			// DMA is receiving to the first half of the buffer
			// need to remove data from the second half
			src = (int16_t *)&i2s_rx_buffer[AUDIO_BLOCK_SAMPLES/2];
			end = (int16_t *)&i2s_rx_buffer[AUDIO_BLOCK_SAMPLES];
			
			if (AudioInputI2S::update_responsibility) AudioStream::update_all();
		}
		else
		{
			// DMA is receiving to the second half of the buffer
			// need to remove data from the first half
			src = (int16_t *)&i2s_rx_buffer[0];
			end = (int16_t *)&i2s_rx_buffer[AUDIO_BLOCK_SAMPLES/2];
		}
		
	
		left = AudioInputI2S::block_left;
		right = AudioInputI2S::block_right;
		if (left != NULL && right != NULL)
		{
			offset = AudioInputI2S::block_offset;
			if (offset <= AUDIO_BLOCK_SAMPLES/2) {
				dest_left = &(left->data[offset]);
				dest_right = &(right->data[offset]);
				AudioInputI2S::block_offset = offset + AUDIO_BLOCK_SAMPLES/2;
				do {
					//n = *src++;
					//*dest_left++ = (int16_t)n;
					//*dest_right++ = (int16_t)(n >> 16);
					*dest_left++ = *src++;
					*dest_right++ = *src++;
				} while (src < end);
			}
		}
		// digitalWriteFast(3, LOW);
	
	#endif 	// !__circle__
}



void AudioInputI2S::update(void)
{
	audio_block_t *new_left=NULL, *new_right=NULL, *out_left=NULL, *out_right=NULL;

	// allocate 2 new blocks, but if one fails, allocate neither
	
	new_left = allocate();
	if (new_left != NULL)
	{
		new_right = allocate();
		if (new_right == NULL)
		{
			#ifdef __circle__
				assert(new_right);
			#endif
			
			release(new_left);
			new_left = NULL;
		}
	}
	#ifdef __circle__
	else if (!new_left)
	{
		static int count = 0;
		if (!count++)
		{
			assert(new_left);
		}
	}
	#endif

	__disable_irq();
	
	// Note that __circle__ will ONLY call update() with full buffers
	// and block_offset will always be >= AUDIO_BLOCK_SAMPLES at this point
	
	if (block_offset >= AUDIO_BLOCK_SAMPLES)
	{
		// the DMA filled 2 blocks, so grab them and get the
		// 2 new blocks to the DMA, as quickly as possible,
		
		// prh - in circle there are "just" user blocks and
		// not directly passed to the DMA routines

		out_left = block_left;
		block_left = new_left;
		out_right = block_right;
		block_right = new_right;
		block_offset = 0;
		__enable_irq();
	
		// then transmit the DMA's former blocks
	
		transmit(out_left, 0);
		release(out_left);
		transmit(out_right, 1);
		release(out_right);
		//Serial.print(".");
	}
	
	// none of these cases should ever happen on __circle__
	
	else if (new_left != NULL)
	{
		#ifdef __circle__
			assert("should never get here");
		#endif
		
		// the DMA didn't fill blocks, but we allocated blocks
		if (block_left == NULL) {
			// the DMA doesn't have any blocks to fill, so
			// give it the ones we just allocated
			block_left = new_left;
			block_right = new_right;
			block_offset = 0;
			__enable_irq();
		}
		else
		{
			// the DMA already has blocks, doesn't need these
			__enable_irq();
			release(new_left);
			release(new_right);
		}
	}
	else
	{
		#ifdef __circle__
			assert("should never get here");
		#endif

		// The DMA didn't fill blocks, and we could not allocate
		// memory... the system is likely starving for memory!
		// Sadly, there's nothing we can do.
		__enable_irq();
	}
	
}


/******************************************************************/


void AudioInputI2Sslave::begin(void)
{
	#ifdef __circle__
	
		I2I_LOG("slave begin()",0);
		AudioOutputI2Sslave::config_i2s();
		update_responsibility = update_setup();
		bcm_pcm.start();
		I2I_LOG("slave begin() finished",0);
	
	#else	// !__circle__
	
		dma.begin(true); // Allocate the DMA channel first
	
		//block_left_1st = NULL;
		//block_right_1st = NULL;
	
		AudioOutputI2Sslave::config_i2s();
	
		CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0
		#if defined(KINETISK)
			dma.TCD->SADDR = (void *)((uint32_t)&I2S0_RDR0 + 2);
			dma.TCD->SOFF = 0;
			dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
			dma.TCD->NBYTES_MLNO = 2;
			dma.TCD->SLAST = 0;
			dma.TCD->DADDR = i2s_rx_buffer;
			dma.TCD->DOFF = 2;
			dma.TCD->CITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;
			dma.TCD->DLASTSGA = -sizeof(i2s_rx_buffer);
			dma.TCD->BITER_ELINKNO = sizeof(i2s_rx_buffer) / 2;
			dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
		#endif
		dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);
		update_responsibility = update_setup();
		dma.enable();
	
		I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
		I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE; // TX clock enable, because sync'd to TX
		dma.attachInterrupt(isr);
		
	#endif	// !__circle__
}


