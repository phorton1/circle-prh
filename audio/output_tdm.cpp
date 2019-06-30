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
#include "output_tdm.h"
#include "AudioSystem.h"
#include <circle/logger.h>

#define log_name "tdmo"

#if 1
	#define TDO_LOG(f,...)           CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
#else
	#define TDO_LOG(...)
#endif


audio_block_t * AudioOutputTDM::s_block_input[NUM_TDM_CHANNELS] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};


bool AudioOutputTDM::s_update_responsibility = false;



void AudioOutputTDM::start(void)
{
	TDO_LOG("start()",0);
	for (int i=0; i < NUM_TDM_CHANNELS; i++)
		s_block_input[i] = NULL;
	bcm_pcm.init();
	s_update_responsibility = AudioSystem::takeUpdateResponsibility();
	bcm_pcm.start();
	TDO_LOG("start() finished",0);	
}


void AudioOutputTDM::isr(void)
{
	if (s_update_responsibility)
		AudioSystem::startUpdate();
	
	// interleave the data to the "block_input" blocks, if any
	// into the output buffer waiting in the bcm_pcm
	
	int16_t *src[NUM_TDM_CHANNELS];
	for (u8 i=0; i<NUM_TDM_CHANNELS; i++)
		src[i] = s_block_input[i] ? s_block_input[i]->data : 0;

	// get the uint32 'ready' input dma buffer from the bcm_pcm
	// and move interleaved bytes from it to the incoming blocks
	
	u16 len = AUDIO_BLOCK_SAMPLES;
	uint32_t *dest = bcm_pcm.getOutBuffer();
	
	while (len--)
	{
		for (u8 i=0; i<NUM_TDM_CHANNELS; i++)
			*dest++ = src[i] ? *(uint32_t *) (src[i]++) : 0;
	}
}



void AudioOutputTDM::update(void)
{
	audio_block_t *prev[NUM_TDM_CHANNELS];
	unsigned int i;

	__disable_irq();
	for (i=0; i < NUM_TDM_CHANNELS; i++)
	{
		prev[i] = s_block_input[i];
		s_block_input[i] = receiveReadOnly(i);
	}
	__enable_irq();
	
	for (i=0; i < NUM_TDM_CHANNELS; i++)
		if (prev[i]) AudioSystem::release(prev[i]);
}


