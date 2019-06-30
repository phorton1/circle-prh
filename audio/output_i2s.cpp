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
#include "output_i2s.h"
#include "AudioSystem.h"
#include "bcm_pcm.h"
#include <circle/logger.h>

#define log_name "i2so"
	
#if 0
	#define I2O_LOG(f,...)           CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
#else
	#define I2O_LOG(...)
#endif
	

audio_block_t * AudioOutputI2S::s_block_left_1st = NULL;
audio_block_t * AudioOutputI2S::s_block_right_1st = NULL;
audio_block_t * AudioOutputI2S::s_block_left_2nd = NULL;
audio_block_t * AudioOutputI2S::s_block_right_2nd = NULL;
bool AudioOutputI2S::s_update_responsibility = false;


void AudioOutputI2S::start()
{
	I2O_LOG("start()",0);
	s_block_left_1st = NULL;
	s_block_right_1st = NULL;
	bcm_pcm.init();
	s_update_responsibility = AudioSystem::takeUpdateResponsibility();
	bcm_pcm.start();
	I2O_LOG("start() finished",0);
}


void AudioOutputI2S::isr(void)
{
	// get a pointer to the buffer that we need to fill in
	// before the next DMA interrupt
	
	u16 len = AUDIO_BLOCK_SAMPLES;
	uint32_t *dest = bcm_pcm.getOutBuffer();
	
	// if we have update responsibility, call the client update methods
	
	if (s_update_responsibility)
		AudioSystem::startUpdate();
	
	// note that the bcm_pcm block size is such that it can
	// hold a full teensy LR block.  Therefore, unlike the
	// teensy code that acts on HALF of a block at a time,
	// we always take a full left or right block at this point,
	// and the "block_offsets" are always zero.
	// existing teensy code()
	
	audio_block_t *blockL = s_block_left_1st;
	audio_block_t *blockR = s_block_right_1st;
	
	if (blockL && blockR)
	{
		int16_t *lptr = blockL->data;
		int16_t *rptr = blockR->data;
		while (len--)
		{
			*dest++ = *(uint32_t *) lptr++;
			*dest++ = *(uint32_t *) rptr++;
		}
		s_block_left_1st = s_block_left_2nd;
		s_block_right_1st = s_block_right_2nd;
		s_block_left_2nd = NULL;
		s_block_right_2nd = NULL;
		AudioSystem::release(blockL);
		AudioSystem::release(blockR);
	}
	else if (blockL)
	{
		int16_t *lptr = blockL->data;
		while (len--)
		{
			*dest++ = *(uint32_t *) lptr++;
			*dest++ = 0;
		}
		s_block_left_1st = s_block_left_2nd;
		s_block_left_2nd = NULL;
		AudioSystem::release(blockL);
	}
	else if (blockR)
	{
		int16_t *rptr = blockR->data;
		while (len--)
		{
			*dest++ = 0;
			*dest++ = *(uint32_t *) rptr++;
		}
		s_block_right_1st = s_block_right_2nd;
		s_block_right_2nd = NULL;
		AudioSystem::release(blockR);
	}
	else
	{
		memset(dest,0,AUDIO_BLOCK_SAMPLES * 4);
	}

	// this routine MUST complete before the DMA issues
	// the next interrupt!! 
}




void AudioOutputI2S::update(void)
{
	audio_block_t *block;
	block = receiveReadOnly(0); // input 0 = left channel
	if (block)
	{
		__disable_irq();
		
		if (s_block_left_1st == NULL)
		{
			s_block_left_1st = block;
			__enable_irq();
		}
		else if (s_block_left_2nd == NULL)
		{
			s_block_left_2nd = block;
			__enable_irq();
		}
		else
		{
			audio_block_t *tmp = s_block_left_1st;
			s_block_left_1st = s_block_left_2nd;
			s_block_left_2nd = block;
			__enable_irq();
			AudioSystem::release(tmp);
		}
	}
	
	block = receiveReadOnly(1); // input 1 = right channel
	if (block)
	{
		__disable_irq();
		if (s_block_right_1st == NULL)
		{
			s_block_right_1st = block;
			__enable_irq();
		}
		else if (s_block_right_2nd == NULL)
		{
			s_block_right_2nd = block;
			__enable_irq();
		}
		else
		{
			audio_block_t *tmp = s_block_right_1st;
			s_block_right_1st = s_block_right_2nd;
			s_block_right_2nd = block;
			__enable_irq();
			AudioSystem::release(tmp);
		}
	}
}





