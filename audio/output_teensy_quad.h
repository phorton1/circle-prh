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
 *
 * See comment in input_teensy_quad.h
 * 
 */

#ifndef output_teensy_quad_h_
#define output_teensy_quad_h_

#include "Arduino.h"
#include "AudioStream.h"


class AudioOutputTeensyQuad : public AudioStream
{
public:
    
	AudioOutputTeensyQuad(void);
	virtual const char *getName() 	{ return "tquado"; }
	virtual u16   getType()  		{ return AUDIO_DEVICE_OUTPUT; }

private:
	friend class AudioInputTeensyQuad;

	static void isr(void);
	static void config_i2s(void);
	
	static bool s_update_responsibility;

	static audio_block_t *s_block_left_1st;
	static audio_block_t *s_block_right_1st;
	
	static audio_block_t *s_block_left_2nd;
	static audio_block_t *s_block_right_2nd;
	audio_block_t *inputQueueArray[2];
	
	virtual void start(void);
	virtual void update(void);
	
};

#endif
