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
#include "control_wm8731.h"
#include "Wire.h"

#ifdef __circle__
    #include <audio/bcm_pcm.h>
	#include <circle/logger.h>
	#define log_name "wm8731"
#else
	#define LOG(...)
#endif


#define WM8731_I2C_ADDR 0x1A
//#define WM8731_I2C_ADDR 0x1B

#define WM8731_REG_LLINEIN	0
#define WM8731_REG_RLINEIN	1
#define WM8731_REG_LHEADOUT	2
#define WM8731_REG_RHEADOUT	3
#define WM8731_REG_ANALOG	4
#define WM8731_REG_DIGITAL	5
#define WM8731_REG_POWERDOWN	6
#define WM8731_REG_INTERFACE	7
#define WM8731_REG_SAMPLING	8
#define WM8731_REG_ACTIVE	9
#define WM8731_REG_RESET	15



#ifdef __circle__
	AudioControlWM8731::AudioControlWM8731()
	{
		// the frame parameters for this were determined empirically.
		// the rpi does better as a slave to the wm8731 than as a master
		// 2,2 channel offsets is definitely non-standard and a kludge,
		// but was required to get even nominally good sound as master.
		// Everything looks correct on the logic analayzer, I'm not sure
		// what is going on here.
		
		bcm_pcm.static_init(
			false,			// bcm_pcm is master device
			44100,          // sample_rate
			16,             // sample_size
			2,              // num_channels
			32,             // channel_width
			2,2,            // channel offsets 2,2 kludge required for master bcm_pcm
			0,				// don't invert BCLK
			1,              // do invert FCLK
			0);             // no callback to start the clock
	}                       
#endif


bool AudioControlWM8731::enable(void)
{
	LOG("enable()",0);
	
	Wire.begin();
	delay(5);
	//write(WM8731_REG_RESET, 0);

	write(WM8731_REG_INTERFACE, 0x02); // I2S, 16 bit, MCLK slave
	write(WM8731_REG_SAMPLING, 0x20);  // 256*Fs, 44.1 kHz, MCLK/1

	// In order to prevent pops, the DAC should first be soft-muted (DACMU),
	// the output should then be de-selected from the line and headphone output
	// (DACSEL), then the DAC powered down (DACPD).

	write(WM8731_REG_DIGITAL, 0x08);   // DAC soft mute
	write(WM8731_REG_ANALOG, 0x00);    // disable all

	write(WM8731_REG_POWERDOWN, 0x00); // codec powerdown

	write(WM8731_REG_LHEADOUT, 0x80);      // volume off
	write(WM8731_REG_RHEADOUT, 0x80);

	delay(100); // how long to power up?

	write(WM8731_REG_ACTIVE, 1);
	delay(5);
	write(WM8731_REG_DIGITAL, 0x00);   // DAC unmuted
	write(WM8731_REG_ANALOG, 0x10);    // DAC selected
	
	// LOG("enable() finished",0);
	return true;
}


bool AudioControlWM8731::write(unsigned int reg, unsigned int val)
{
	Wire.beginTransmission(WM8731_I2C_ADDR);
	Wire.write((reg << 1) | ((val >> 8) & 1));
	Wire.write(val & 0xFF);
	Wire.endTransmission();
	return true;
}

bool AudioControlWM8731::volumeInteger(unsigned int n)
{
	// LOG("volumeInteger(%d)",n);

	// n = 127 for max volume (+6 dB)
	// n = 48 for min volume (-73 dB)
	// n = 0 to 47 for mute
	if (n > 127) n = 127;
	 //Serial.print("volumeInteger, n = ");
	 //Serial.println(n);
	write(WM8731_REG_LHEADOUT, n | 0x180);
	write(WM8731_REG_RHEADOUT, n | 0x80);
	return true;
}

bool AudioControlWM8731::inputLevel(float n)
{
	// LOG("inputLevel(%0.2f)",n);

	// range is 0x00 (min) - 0x1F (max)
	int _level = int(n * 31.f); 
	_level = _level > 0x1F ? 0x1F : _level;
	write(WM8731_REG_LLINEIN, _level);
	write(WM8731_REG_RLINEIN, _level);
	return true;
}

bool AudioControlWM8731::inputSelect(int n)
{
	// LOG("inputSelect(%d)",n);

	if (n == AUDIO_INPUT_LINEIN) {
		write(WM8731_REG_ANALOG, 0x12);
	} else if (n == AUDIO_INPUT_MIC) {
		write(WM8731_REG_ANALOG, 0x15);
	} else {
		return false;
	}
	return true;
}




/******************************************************************/

#ifdef __circle__
	AudioControlWM8731master::AudioControlWM8731master()
	{
		// the rpi as a slave (wm8731 as master) works better
		// these are, to the best of my knowledge, the standard
		// i2s frame settings ... 1 bit offsets without inverting
		// clocks.
		
		bcm_pcm.static_init(
			true,			// bcm_pcm is slave device
			44100,          // sample_rate
			16,             // sample_size
			2,              // num_channels
			32,             // channel_width
			1,1,            // normal offset settings if the wm8731 is the master clock
			0,				// don't invert BCLK
			0,              // don't invert FCLK
			0);             // no callback to start the clock		
	}
#endif



bool AudioControlWM8731master::enable(void)
{
	LOG("master enable()",0);
	
	Wire.begin();
	delay(5);
	//write(WM8731_REG_RESET, 0);

	write(WM8731_REG_INTERFACE, 0x42); // 0z02=I2S, 0z04=MCLK master,
		// 16 bit is default 0
	write(WM8731_REG_SAMPLING, 0x20);  // 256*Fs, 44.1 kHz, MCLK/1
		// 0x20 == 0x8<<2 with BOSR==0

	// In order to prevent pops, the DAC should first be soft-muted (DACMU),
	// the output should then be de-selected from the line and headphone output
	// (DACSEL), then the DAC powered down (DACPD).

	write(WM8731_REG_DIGITAL, 0x08);   // DAC soft mute
	write(WM8731_REG_ANALOG, 0x00);    // disable all
	write(WM8731_REG_POWERDOWN, 0x00); // codec powerdown
		
	write(WM8731_REG_LHEADOUT, 0x80);      // volume off
		// 0x80 = WM8731_HEADOUT_ZCEN
	write(WM8731_REG_RHEADOUT, 0x80);

	delay(100); // how long to power up?

	write(WM8731_REG_ACTIVE, 1);
	delay(5);
		
	write(WM8731_REG_DIGITAL, 0x00);   // DAC unmuted
	write(WM8731_REG_ANALOG, 0x10);    // DAC selected
		// 0x10 = WM8731_ANALOG_DACSEL
	
	// LOG("master enable() finished",0);
	return true;
}



