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
#include "AudioSystem.h"
#include "input_i2s.h"
#include "bcm_pcm.h"
#include <circle/logger.h>


#define log_name "i2si"

#if 0
	#define I2I_LOG(f,...)           CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
#else
	#define I2I_LOG(...)
#endif
	

audio_block_t * AudioInputI2S::s_block_left = NULL;
audio_block_t * AudioInputI2S::s_block_right = NULL;
bool AudioInputI2S::s_update_responsibility = 0;


void AudioInputI2S::start(void)
{
	I2I_LOG("start()",0);
	bcm_pcm.init();
	s_update_responsibility = AudioSystem::takeUpdateResponsibility();
	bcm_pcm.start();
	
	// allocate the initial blocks to
	// prevent assertions in update()
	
	s_block_left = AudioSystem::allocate();
	s_block_right = AudioSystem::allocate();
	assert(s_block_left);
	assert(s_block_right);
	
	I2I_LOG("start() finished",0);
}


void AudioInputI2S::isr(void)
{
	// get the uint32 'ready' input dma buffer from the bcm_pcm
	
	u16 len = AUDIO_BLOCK_SAMPLES;
	uint32_t *src = bcm_pcm.getInBuffer();
	
	// get pointers to the destination (client) int16's and loop
	// moving pairs of u32's from the dma buffer into pairs
	// of int16's in the "client" teensy audio blocks

	audio_block_t *left = s_block_left;
	audio_block_t *right = s_block_right;
	int16_t *dest_left = left ? left->data : 0;
	int16_t *dest_right = right ? right->data : 0;
	
	if (left && right)
	{
		while (len--)
		{
			*dest_left++ = *(int16_t *) src++;
			*dest_right++ = *(int16_t *) src++;
		}
	}
	else if (left)
	{
		while (len--)
		{
			*dest_left++ = *(int16_t *) src++;
			src++;
		}
	}
	else if (right)
	{
		while (len--)
		{
			src++;
			*dest_right++ = *(int16_t *) src++;
		}
	}
	
	// buffer is ready: call the client update method.
	
	if (s_update_responsibility)
		AudioSystem::startUpdate();
			
	// this routine MUST complete before the DMA issues
	// the next interrupt!! 
	
}




void AudioInputI2S::update(void)
{
	static bool notified = 0;
	audio_block_t *new_left	 = NULL;
	audio_block_t *new_right = NULL;
	audio_block_t *out_left  = NULL;
	audio_block_t *out_right = NULL;

	// allocate 2 new blocks, but if one fails, allocate neither
	
	new_left = AudioSystem::allocate();
	if (new_left != NULL)
	{
		new_right = AudioSystem::allocate();
		if (new_right == NULL)
		{
			if (!notified)
			{
				notified = true;
				LOG_ERROR("Could not allocate block",0);
			}
			
			AudioSystem::release(new_left);
			new_left = NULL;
		}
	}
	else if (!new_left)
	{
		if (!notified)
		{
			notified = true;
			LOG_ERROR("Could not allocate block",0);
		}
	}

	
	// Note that __circle__ will ONLY call update() with full buffers

	__disable_irq();
	out_left = s_block_left;		// grab the block filled in by DMA
	s_block_left = new_left;		// give it a new one
	out_right = s_block_right;
	s_block_right = new_right;
	__enable_irq();
	
	// then transmit the DMA's former blocks

	transmit(out_left, 0);					// send it to everyone
	AudioSystem::release(out_left);			// and we release it
	transmit(out_right, 1);
	AudioSystem::release(out_right);

}



