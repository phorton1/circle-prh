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

#ifndef _input_i2s_h_
#define _input_i2s_h_

#include "Arduino.h"
#include "AudioStream.h"
#include "bcm_pcm.h"

// there is only a single device on circle
// whether or not it is a slave is determined
// by the codec start() method

class AudioInputI2S : public AudioStream
{
public:
	
	AudioInputI2S(void) : AudioStream(0,2,NULL)
    {
		bcm_pcm.setInISR(isr);
    }

	virtual const char *getName() 	{ return "i2si"; }
	virtual u16   getType()  		{ return AUDIO_DEVICE_INPUT; }
	
protected:

	virtual void start();
	static void isr(void);
	static bool s_update_responsibility;
	
private:

	virtual void update(void);
	
	static audio_block_t *s_block_left;
	static audio_block_t *s_block_right;
	
};


#endif
