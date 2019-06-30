/* Audio Library for Teensy 3.X
 * Copyright (c) 2017, Paul Stoffregen, paul@pjrc.com
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
#include "input_tdm.h"
#include "AudioSystem.h"
#include <circle/logger.h>

#define log_name "tdmi"

#if 1
	#define TDI_LOG(f,...)           CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
#else
	#define TDI_LOG(...)
#endif


bool AudioInputTDM::s_update_responsibility = false;

audio_block_t *AudioInputTDM::s_block_incoming[NUM_TDM_CHANNELS] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};



void AudioInputTDM::start(void)
{
	TDI_LOG("start()",0);
	bcm_pcm.init();
	s_update_responsibility = AudioSystem::takeUpdateResponsibility();
	bcm_pcm.start();
	TDI_LOG("start() finished",0);		
}



void AudioInputTDM::isr(void)
{
	// de-interleave the data to the "incoming blocks" if the
	// blocks have been allocated (by at least one previous call
	// to update())
	
	if (s_block_incoming[0] != NULL)
	{
		int16_t *dest[NUM_TDM_CHANNELS];
		for (u8 i=0; i<NUM_TDM_CHANNELS; i++)
			dest[i] = s_block_incoming[i]->data;

		// get the uint32 'ready' input dma buffer from the bcm_pcm
		// and move interleaved bytes from it to the incoming blocks
		
		u16 len = AUDIO_BLOCK_SAMPLES;
		uint32_t *src = bcm_pcm.getInBuffer();
		
		while (len--)
		{
			for (u8 i=0; i<NUM_TDM_CHANNELS; i++)
				*(dest[i])++ = *(int16_t *) src++;
		}
	}

	if (s_update_responsibility)
		AudioSystem::startUpdate();
		
}


void AudioInputTDM::update(void)
{
	unsigned int i, j;
	audio_block_t *new_block[NUM_TDM_CHANNELS];
	audio_block_t *out_block[NUM_TDM_CHANNELS];

	// allocate 16 new blocks.  If any fails, allocate none
	
	for (i=0; i < NUM_TDM_CHANNELS; i++)
	{
		new_block[i] = AudioSystem::allocate();
		if (new_block[i] == NULL)
		{
			for (j=0; j < i; j++)
				AudioSystem::release(new_block[j]);
			memset(new_block, 0, sizeof(new_block));
			break;
		}
	}
	
	__disable_irq();
	memcpy(out_block, s_block_incoming, sizeof(out_block));
	memcpy(s_block_incoming, new_block, sizeof(s_block_incoming));
	__enable_irq();
 	
	if (out_block[0] != NULL)
	{
		// if we got 1 block, all 16 are filled
		for (i=0; i < NUM_TDM_CHANNELS; i++)
		{
			transmit(out_block[i], i);
			AudioSystem::release(out_block[i]);
		}
	}
}

