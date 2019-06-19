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

// This circle only class is kind of neat.
//
// It adds 2 channel input and output device(s) that can communicate
// with an existing, running, teensy audio application.  The teensy
// MPU has two i2s modules (as opposed to the one on the bm2835) that
// run synchronously.   Paul has created the AudioI2SQuad input and
// output devices to allow two teensy sound shields to be stacked for
// quad io.
//
// This class allows you to easily use any rPi, including the zero and 3B+,
// as a DSP co-processor to a teensy audio application.  It *may* even allow
// you to retrofit such an additional processor into your existing teensy
// audio application with a minimal set of changes.
//
// From the rpi's perspective, it is simply an i2s IO device, that
// happens to use a 16 bit channel width instead of 32 bits, aa that
// is the channel frame packing that I see with the default build of
// the teensy quad libraries in the Arduino IDE.  At this time there
// is no corresponding "control" object, though one could be implemented
// in the teensy INO sketch, if you really wanted.
//
//
//  +-------------------+	  teensy        +--------------------+
//  |   Teensy Audio    |   quad channels   |       rPi          |
//  |    application    |      2 & 3        | acting as an audio |
//  |    using quad     | <===============> |    co-processor    |
//  |    audio device   |                   |                    |
//  +-------------------+					+--------------------+
//       ^        |
//       |        |      teensy quad 
//       |        v     channels 0 & 1
//      IN       OUT
//
// Connections:
//
// You connect following rPi pins to the teensy audio shield.
// The audio shield pins are given in terms of their teensy
// equivilant gpio pin number.
//
//      rPi   gpio   Teensy 3.2_quad  3.6_quad   normal_teensy_i2s
//
//	    BCLK    18   <----  9          9            9
//	    FCLK    19   <----  23         23           23
//	    RXD     22   ---->  15 tx1     15 tx1       22 tx0       
//	    TXD     21   <----  30 rx1     38 rx1       13 rx0   
//
// You have to remove the small capicator attached to pin15 on the teensy audio shield.
// Unfortunately, pin30 is on the underside of a teensy3.2.
// On the teensy 3.6, the 2nd i2s rx channel is on pin38, which is breadboard-able.

#ifndef _input_teensy_quad_h_
#define _input_teensy_quad_h_

#include "Arduino.h"
#include "AudioStream.h"

	
class AudioInputTeensyQuad : public AudioStream
	// this is a slave only device
{
public:

	AudioInputTeensyQuad(void);
	virtual void update(void);
	void begin(void);

protected:	

	static bool update_responsibility;
	static void isr(void);

private:

	static audio_block_t *block_left;
	static audio_block_t *block_right;
};

#endif
