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
//--------------------------------------------------------------
// Pin Numbers
//--------------------------------------------------------------
// Start off by noting that the RX and TX labels on the audio
// 		shield diagrams are from the perspective of the TEENSY, not
// 		from the perspective of the audio shields.  So the pin labelled
// 		TX on the audio shield diagram is actually the SGTL5000 rx pin.
// All pin numbers are in terms of their GPIO number.
//
// The pin numbers for various I2S devices are best found
// 		Paul's audio library design tool.  The main i2s channel
// 		is as follows
//
// functional										Optimized Quad
// name			teensy4.x 			teensy3.x		New Name
//-----------------------------------------------------------
// MCLK1		23_A9_MCLK			11_MOSI0		I2S_MCLK
// BCLK1		21_A7_RX5_BCLK1		9_RX2_CS01		I2S_BCLK
// LRCLCK1		20_A6_TX5_LRCLK1	23_A9			I2S_FCLK
// TX1A			7_RX2_OUT1A			22_A8			I2S_TXA
// RX1A			8_TX2_IN1			13_LED_SCK0		I2S_RXA
//
//
// The teensyQuad devices adds an additional set of TX1B and RX1B pins
// that are sent to the rPi. Note that the yellow teensy 4.1 i2s labels
// don't necessarily match up to the device definition, in that teensy
// RX2A is using 6_OUT1D according to the 4.1 pinout diagram.
//
//				functional												Optimized Quad
// RPI			name		teensy4.x		teensy3.6	teensy3.2		New Name
//--------------------------------------------------------------------------------------
// 18_BCLK		BCLK1		duplicated to RPI
// 19_FCLK		LRCLK1		duplicated to RPI
// 20_RXD		TX1B		32_OUT1B		15_A1		15_A1			I2S_RXB
// 21_TXD		RX1B		6_OUT1D			38_A9		30 on bottom	I2S_TXB
//
// For grins and giggles, here is the separate I2S2 device pin mappings
// only available on the teensy 4.x series:
//
// name			teensy 4.x
//--------------------------
// MCLK2		33_MCLK2
// BCLK2		4_BCLK2
// LRCLCK2		3_LRCLK2
// TX2A			2_OUT2
// RX2A			5_IN2


#ifndef _input_teensy_quad_h_
#define _input_teensy_quad_h_

#include "Arduino.h"
#include "AudioStream.h"

	
class AudioInputTeensyQuad : public AudioStream
	// this is a slave only device
{
public:

	AudioInputTeensyQuad(void);

    virtual const char *getName()   { return "tquadi"; }        
	virtual u16   getType()  		{ return AUDIO_DEVICE_INPUT; }
	
private:

	static bool s_update_responsibility;
	static void isr(void);

	static audio_block_t *s_block_left;
	static audio_block_t *s_block_right;
	
	virtual void update(void);
	void start(void);
	
};

#endif
