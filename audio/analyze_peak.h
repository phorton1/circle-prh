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

#ifndef analyze_peakdetect_h_
#define analyze_peakdetect_h_

#include "Arduino.h"
#include "AudioStream.h"

class AudioAnalyzePeak : public AudioStream
{
public:
	AudioAnalyzePeak(void) : AudioStream(1, inputQueueArray)
	{
		#ifdef __circle__
	        SET_AUDIO_INSTANCE()
			m_count = 0;
			m_ui_rate = 0;
			m_ui_cb = NULL;
			m_ui_param = NULL;
		#endif
		
		min_sample = 32767;
		max_sample = -32768;
	}
	bool available(void) {
		__disable_irq();
		bool flag = new_output;
		if (flag) new_output = false;
		__enable_irq();
		return flag;
	}
	float read(void) {
		__disable_irq();
		int min = min_sample;
		int max = max_sample;
		min_sample = 32767;
		max_sample = -32768;
		__enable_irq();
		min = abs(min);
		max = abs(max);
		if (min > max) max = min;
		return (float)max / 32767.0f;
	}
	float readPeakToPeak(void) {
		__disable_irq();
		int min = min_sample;
		int max = max_sample;
		min_sample = 32767;
		max_sample = -32768;
		__enable_irq();
		return (float)(max - min) / 32767.0f;
	}

	virtual void update(void);
	
    #ifdef __circle__
		virtual const char *dbgName()  { return "peak"; }
		virtual u8 dbgInstance()       { return instance_num; }
		void registerUIHandler(u16 rate, void (*cb)(void *),void *param)
		{
			m_ui_rate  = rate;
			m_ui_cb    = cb;
			m_ui_param = param;
		}
	#endif	
	
private:
	audio_block_t *inputQueueArray[1];
	volatile bool new_output;
	int16_t min_sample;
	int16_t max_sample;
	
    #ifdef __circle__
		static u8 next_instance_num;
		u8 instance_num;
		u16 m_ui_rate;
		u32 m_count;
		void (*m_ui_cb)(void *);
		void *m_ui_param;
	#endif	
	
};

#endif
