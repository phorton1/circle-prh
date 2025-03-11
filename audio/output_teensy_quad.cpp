/* Circle Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 * Copyright (c) 2019, Patrick Horton
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
#include "output_teensy_quad.h"

// circle only

#include "bcm_pcm.h"
#include <circle/logger.h>
#define log_name "teensy_quad_output"
#if 0
	#define QUAD_LOG(f,...)           CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
#else
	#define QUAD_LOG(...)
#endif
	

audio_block_t * AudioOutputTeensyQuad::s_block_left_1st = NULL;
audio_block_t * AudioOutputTeensyQuad::s_block_right_1st = NULL;
audio_block_t * AudioOutputTeensyQuad::s_block_left_2nd = NULL;
audio_block_t * AudioOutputTeensyQuad::s_block_right_2nd = NULL;
bool AudioOutputTeensyQuad::s_update_responsibility = false;


    
AudioOutputTeensyQuad::AudioOutputTeensyQuad(void) :
	AudioStream(2,2, inputQueueArray)
{
	bcm_pcm.setOutISR(isr);
}
    

// static
void AudioOutputTeensyQuad::config_i2s()
{
	// OLD COMMENT: basically the only thing that changes from any other
	// circle i2s device is that the channel width of the
	// teensy quad device is 16, not 32, bits

#if 1 	// 2025-03-10

	// values that work with TE3 as of this writing
	// after hooking up the logic analyzer I immediately
	// saw that the rPI is receiving 32 bit wide channels
	// from the TE3_audio device.  Changing the channel
	// width to 32 below made it start working as far as
	// I can tell.

	// See notes in TE3_audio/docs/audio_bug.md for potential
	// next steps.  This device might be deleted or made meaningful
	// in a different way.

	bcm_pcm.static_init(
		true,			// bcm_pcm is slave device         	-
		44100,          // sample_rate                     	-
		16,             // sample_size                     	-
		2,              // num_channels                    	-
		32,             // channel_width              		- 32 WORKED!!
		1,1,            // channel offsets                 	-
		1,				// invert BCLK                     	-
		0,              // don't invert FCLK               	-
		0);             // no callback to start the clock  	-

#else	// original values
	bcm_pcm.static_init(
		true,			// bcm_pcm is slave device
		44100,          // sample_rate
		16,             // sample_size
		2,              // num_channels
		15,             // channel_width
		1,1,            // channel offsets
		1,				// invert BCLK
		0,              // don't invert FCLK
		0);             // no callback to start the clock
#endif

	bcm_pcm.init();
}


void AudioOutputTeensyQuad::start(void)
{
	QUAD_LOG("start()",0);
	s_block_left_1st = NULL;
	s_block_right_1st = NULL;
	config_i2s();
	s_update_responsibility = AudioSystem::takeUpdateResponsibility();
	bcm_pcm.start();
	QUAD_LOG("start() finished",0);
}


void AudioOutputTeensyQuad::isr(void)
	// this is essentially unchanged from the regular i2s device
	// and could benefit from factoring.  All that chnages in
	// this device is the i2s channel size and initialization
	// responsibility.
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




void AudioOutputTeensyQuad::update(void)
{
	// null audio device: discard all incoming data
	//if (!active) return;
	//audio_block_t *block = receiveReadOnly();
	//if (block) release(block);

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




