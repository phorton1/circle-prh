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

#ifndef control_sgtl5000_h_
#define control_sgtl5000_h_

#include "AudioDevice.h"
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>


class AudioControlSGTL5000 : public AudioCodec
{
public:
	
	AudioControlSGTL5000();
	
	void setAddress(uint8_t level);
	
	virtual const char *getName()  { return "sgtl5000m"; }
	virtual void start();
		// public until AudioSystem starts it ...
	
	
	void volume(float n) { return volumeInteger(n * 129 + 0.499); }
	void muteHeadphone(void) { return write(0x0024, ana_ctrl | (1<<4)); }
	void unmuteHeadphone(void) { return write(0x0024, ana_ctrl & ~(1<<4)); }
	void muteLineout(void) { return write(0x0024, ana_ctrl | (1<<8)); }
	void unmuteLineout(void) { return write(0x0024, ana_ctrl & ~(1<<8)); }
	
	void inputSelect(int n)
	{
		if (n == AUDIO_INPUT_LINEIN)
		{
			write(0x0020, 0x055);				// +7.5dB gain (1.3Vp-p full scale)
			write(0x0024, ana_ctrl | (1<<2)); 	// enable linein
		}
		else if (n == AUDIO_INPUT_MIC)
		{
			write(0x002A, 0x0173);				// mic preamp gain = +40dB
			write(0x0020, 0x088);     			// input gain +12dB (is this enough?)
			write(0x0024, ana_ctrl & ~(1<<2)); 	// enable mic
		}
	}
	
	void volume(float left, float right);
	
	void micGain(unsigned int dB);
	void lineInLevel(uint8_t n) { return lineInLevel(n, n); }
	void lineInLevel(uint8_t left, uint8_t right);
	void lineOutLevel(uint8_t n);
	void lineOutLevel(uint8_t left, uint8_t right);
	void dacVolume(float n);
	void dacVolume(float left, float right);
	void dacVolumeRamp();
	void dacVolumeRampLinear();
	void dacVolumeRampDisable();
	void adcHighPassFilterEnable(void);
	void adcHighPassFilterFreeze(void);
	void adcHighPassFilterDisable(void);
	void audioPreProcessorEnable(void);
	void audioPostProcessorEnable(void);
	void audioProcessorDisable(void);
	void eqFilterCount(uint8_t n);
	void eqSelect(uint8_t n);
	void eqBand(uint8_t bandNum, float n);
	void eqBands(float bass, float mid_bass, float midrange, float mid_treble, float treble);
	void eqBands(float bass, float treble);
	void eqFilter(uint8_t filterNum, int *filterParameters);
	void autoVolumeControl(uint8_t maxGain, uint8_t lbiResponse, uint8_t hardLimit, float threshold, float attack, float decay);
	void autoVolumeEnable(void);
	void autoVolumeDisable(void);
	void enhanceBass(float lr_lev, float bass_lev);
	void enhanceBass(float lr_lev, float bass_lev, uint8_t hpf_bypass, uint8_t cutoff);
	void enhanceBassEnable(void);
	void enhanceBassDisable(void);
	void surroundSound(uint8_t width);
	void surroundSound(uint8_t width, uint8_t select);
	void surroundSoundEnable(void);
	void surroundSoundDisable(void);
	void killAutomation(void) { semi_automated=false; }

protected:
	
	bool muted;
	void volumeInteger(unsigned int n); // range: 0x00 to 0x80
	
	uint16_t ana_ctrl;
	uint8_t i2c_addr;
	
	unsigned char calcVol(float n, unsigned char range);
	unsigned int read(unsigned int reg);
	void write(unsigned int reg, unsigned int val);
	void modify(unsigned int reg, unsigned int val, unsigned int iMask);
	
	void dap_audio_eq_band(uint8_t bandNum, float n);

private:

	bool semi_automated;
	void automate(uint8_t dap, uint8_t eq);
	void automate(uint8_t dap, uint8_t eq, uint8_t filterCount);
	
	// uses GPIO4 to send 11.289Mhz MCLK to the shield

	CGPIOPin   m_MPIN;
	CGPIOClock m_MCLK;
	
};


//For Filter Type: 0 = LPF, 1 = HPF, 2 = BPF, 3 = NOTCH, 4 = PeakingEQ, 5 = LowShelf, 6 = HighShelf
  #define FILTER_LOPASS 0
  #define FILTER_HIPASS 1
  #define FILTER_BANDPASS 2
  #define FILTER_NOTCH 3
  #define FILTER_PARAEQ 4
  #define FILTER_LOSHELF 5
  #define FILTER_HISHELF 6
  
//For frequency adjustment
  #define FLAT_FREQUENCY 0
  #define PARAMETRIC_EQUALIZER 1
  #define TONE_CONTROLS 2
  #define GRAPHIC_EQUALIZER 3


void calcBiquad(uint8_t filtertype, float fC, float dB_Gain, float Q, uint32_t quantization_unit, uint32_t fS, int *coef);


#endif 	// !control_sgtl5000_h_
