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

#ifndef control_cs42448_h_
#define control_cs42448_h_

#include "AudioDevice.h"
#include <math.h>


#define AUDIO_INJECTOR_OCTO 
	// The only cs42448 device I have is the Audio Injector Octo
	// It uses an additional 5 gpio pins for reset and rate setting.
	// Theoretically this is a generic cs42448 device if OCTO is
	// not defined, but that has not been tested.


class AudioControlCS42448 : public AudioCodec
{
public:

	AudioControlCS42448();
	
	virtual const char *getName()  { return "cs42448"; }
	
	virtual void volume(float level) override;
	virtual void inputLevel(float level) override;

	void volume(int channel, float level);
    
	void inputLevel(int channel, float level);
        
	virtual void start();
		// public until AudioSystem starts it ...
	
private:
	
	u8   m_muteMask;

	u8 read(u8 address);
	void write(uint32_t address, uint32_t data);
	void write(uint32_t address, const void *data, uint32_t len, bool auto_inc=true);

	#ifdef AUDIO_INJECTOR_OCTO
		static void reset();
		static void startClock();
	#endif
	
};

#endif
