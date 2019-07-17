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

#ifndef control_wm8731_h_
#define control_wm8731_h_

#include "AudioDevice.h"


class AudioControlWM8731 : public AudioCodec
{
public:
    
    AudioControlWM8731();
    
    virtual const char *getName() { return "wm8731"; }
	virtual void start();
		// public until AudioSystem starts it ...

	void volume(float n) { return volumeInteger(n * 80.0 + 47.499); }
	void inputLevel(float n); // range: 0.0f to 1.0f
	void inputSelect(int n);
    
protected:
    
    AudioControlWM8731(u16 dummy) {}
	void write(unsigned int reg, unsigned int val);
    
	virtual u32 handleEvent(systemEvent *event);
    
    
private:
    
	void volumeInteger(unsigned int n); // range: 0x2F to 0x7F

};



class AudioControlWM8731Slave : public AudioControlWM8731
{
public:

    AudioControlWM8731Slave();

    virtual const char *getName() { return "wm8731s"; }
    
private:
    
	virtual void start();

};


#endif
