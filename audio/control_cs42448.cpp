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
#include "control_cs42448.h"
#include <circle/timer.h>
#include <circle/gpiopin.h>
#include <circle/logger.h>
#include <audio/bcm_pcm.h>
#include "Wire.h"


#define log_name "cm42448"

#define I2C_ADDR  0x48


#ifdef AUDIO_INJECTOR_OCTO

    // Note: for the Audio Injector Octo both the bcm and the codec
    // are set as slaves, and you wake up the fpga with reset() and
    // setSampleRate() for it to become the clock master.

	#define OCTO_PIN_RESET	5
	#define OCTO_PIN_MULT0  27
	#define OCTO_PIN_MULT1  22
	#define OCTO_PIN_MULT2  23
	#define OCTO_PIN_MULT3  24

    CGPIOPin *octo_reset;
    CGPIOPin *octo_mult[4];
    u32  g_sample_rate;

#endif


#define CS42448_Chip_ID				    0x01
#define CS42448_Power_Control			0x02
#define CS42448_Functional_Mode			0x03
#define CS42448_Interface_Formats		0x04
#define CS42448_ADC_Control	            0x05
#define CS42448_Transition_Control		0x06
#define CS42448_DAC_Channel_Mute		0x07
#define CS42448_AOUT1_Volume_Control	0x08
#define CS42448_AOUT2_Volume_Control	0x09
#define CS42448_AOUT3_Volume_Control	0x0A
#define CS42448_AOUT4_Volume_Control	0x0B
#define CS42448_AOUT5_Volume_Control	0x0C
#define CS42448_AOUT6_Volume_Control	0x0D
#define CS42448_AOUT7_Volume_Control	0x0E
#define CS42448_AOUT8_Volume_Control	0x0F
#define CS42448_DAC_Channel_Invert		0x10
#define CS42448_AIN1_Volume_Control		0x11
#define CS42448_AIN2_Volume_Control		0x12
#define CS42448_AIN3_Volume_Control		0x13
#define CS42448_AIN4_Volume_Control		0x14
#define CS42448_AIN5_Volume_Control		0x15
#define CS42448_AIN6_Volume_Control		0x16
#define CS42448_ADC_Channel_Invert		0x17
#define CS42448_Status_Control			0x18
#define CS42448_Status				    0x19
#define CS42448_Status_Mask			    0x1A
#define CS42448_MUTEC_Pin_Control		0x1B

// register values

#define FM_MCLK_1_TO_12_MHZ             (0x0 << 1)
#define FM_MCLK_2_TO_25_MHZ             (0x2 << 1)
#define FM_MCLK_4_TO_51_MHZ             (0x4 << 1)

#define FM_ADC_SPEED_SINGLE             (0x0 << 4)
#define FM_ADC_SPEED_DOUBLE             (0x1 << 4)
#define FM_ADC_SPEED_QUAD               (0x2 << 4)
#define FM_ADC_SPEED_AUTO_SLAVE         (0x3 << 4)

#define FM_DAC_SPEED_SINGLE             (0x0 << 6)
#define FM_DAC_SPEED_DOUBLE             (0x1 << 6)
#define FM_DAC_SPEED_QUAD               (0x2 << 6)
#define FM_DAC_SPEED_AUTO_SLAVE         (0x3 << 6)

#define INTFC_ADC_LEFTJ                 (0 << 0)
#define INTFC_ADC_I2S                   (1 << 0)
#define INTFC_ADC_RJUST24               (2 << 0)
#define INTFC_ADC_RJUST16               (3 << 0)
#define INTFC_ADC_ONELINE_1             (4 << 0)
#define INTFC_ADC_ONELINE_2             (5 << 0)
#define INTFC_ADC_TDM                   (6 << 0)

#define INTFC_DAC_LEFTJ                 (0 << 3)
#define INTFC_DAC_I2S                   (1 << 3)
#define INTFC_DAC_RJUST24               (2 << 3)
#define INTFC_DAC_RJUST16               (3 << 3)
#define INTFC_DAC_ONELINE_1             (4 << 3)
#define INTFC_DAC_ONELINE_2             (5 << 3)
#define INTFC_DAC_TDM                   (6 << 3)

#define INTFC_AUX_LEFTJ                 (0 << 6)
#define INTFC_AUX_I2S                   (1 << 6)

#define INTFC_FREEZE                    (1 << 7)

#define ADCC_AIN6_MUX                   (1 << 0)
#define ADCC_AIN5_MUX                   (1 << 1)
#define ADCC_ADC3_SINGLE                (1 << 2)
#define ADCC_ADC2_SINGLE                (1 << 3)
#define ADCC_ADC1_SINGLE                (1 << 4)
#define ADCC_DAC_DEM                    (1 << 5)
#define ADCC_ADC3_HPF_FREEZE            (1 << 6)
#define ADCC_ADC12_HPF_FREEZE           (1 << 7)

#define TRANS_ADC_SZC0                  (1 << 0)
#define TRANS_ADC_SZC1                  (1 << 1)
#define TRANS_ADC_SNGVOL                (1 << 2)
#define TRANS_MUTE_ADC_SP               (1 << 3)
#define TRANS_AMUTE                     (1 << 4)
#define TRANS_DAC_SZC0                  (1 << 5)
#define TRANS_DAC_SZC1                  (1 << 6)
#define TRANS_DAC_SNGVOL                (1 << 7)


//------------------------------------------------------------------------
// construction and initialization
//------------------------------------------------------------------------

AudioControlCS42448::AudioControlCS42448(void)
{
	m_muteMask = 0xff;
		// start with all channels un-muted
		// should match setup bytes below

	#ifdef AUDIO_INJECTOR_OCTO
		g_sample_rate = 44100;
	#endif

	bcm_pcm.static_init(
		true,               // bcm_pcm is slave device
		g_sample_rate,      // sample_rate
		16,                 // sample_size
		8,                  // num_channels
		32,                 // channel_width
		1,1,                // channel offsets must be zero for Octo
		0,                  // don't invert BCLK
		1,                  // do invert FCLK
		&startClock);       // callback to start the clock

	// This is not true TDM, but, rather, an Octo specific I2s variant,
	// synchronized by starting the clock correctly.
}



static const uint8_t default_config[] =
{
#ifdef AUDIO_INJECTOR_OCTO
    FM_ADC_SPEED_AUTO_SLAVE | FM_DAC_SPEED_AUTO_SLAVE | FM_MCLK_4_TO_51_MHZ,
        // CS42448_Functional_Mode = 0xF9 = slave mode, MCLK 51.2 MHz max
#else
    // slave does not work with Octo
    FM_ADC_SPEED_AUTO_SLAVE | FM_DAC_SPEED_AUTO_SLAVE | FM_MCLK_2_TO_25_MHZ,
        // CS42448_Functional_Mode = 0xF6 = slave mode, MCLK 25.6 MHz max
#endif

    INTFC_ADC_TDM | INTFC_DAC_TDM | INTFC_AUX_I2S,
        // CS42448_Interface_Formats = T0x76 = DM mode + aux I2s
    ADCC_ADC3_SINGLE | ADCC_ADC2_SINGLE | ADCC_ADC1_SINGLE,
        // CS42448_ADC_Control_DAC_DeEmphasis = 0x1C = single ended ADC
    TRANS_ADC_SZC0 | TRANS_ADC_SZC1 | TRANS_DAC_SZC0 | TRANS_DAC_SZC1,
        // CS42448_Transition_Control = 0x63 = soft vol control

    0xff    // CS42448_DAC_Channel_Mute = 0xff = all outputs mute
};



void AudioControlCS42448::start(void)
{
	LOG("start()",0);

    #ifdef AUDIO_INJECTOR_OCTO
        reset();
    #endif

	Wire.begin();

	// __circle__ sanity check code to make sure I am talking i2c
	// My chip revision is 0x04 instead of documented 0x01

	LOG("0x01. Chip_ID            = 0x%02x",read(CS42448_Chip_ID));

	#if 0
		LOG("0x02. Power_Control      = 0x%02x",read(CS42448_Power_Control));
		LOG("0x03. Functional_Mode    = 0x%02x",read(CS42448_Functional_Mode));
		LOG("0x04. Interface_Formats  = 0x%02x",read(CS42448_Interface_Formats));
		LOG("0x05. ADC_Control        = 0x%02x",read(CS42448_ADC_Control));
		LOG("0x06. Transition_Control = 0x%02x",read(CS42448_Transition_Control));
		LOG("0x07. DAC_Channel_Mute   = 0x%02x",read(CS42448_DAC_Channel_Mute));
		LOG("0x10. DAC_Channel_Invert = 0x%02x",read(CS42448_DAC_Channel_Invert));
		LOG("0x17. ADC_Channel_Invert = 0x%02x",read(CS42448_ADC_Channel_Invert));
		LOG("0x18. Status_Control     = 0x%02x",read(CS42448_Status_Control));
		LOG("0x19. Status             = 0x%02x",read(CS42448_Status));
		LOG("0x1A. Status_Mask        = 0x%02x",read(CS42448_Status_Mask));
		LOG("0x1B. MUTEC_Pin_Control  = 0x%02x",read(CS42448_MUTEC_Pin_Control));
		// test a read write operation
		if (!write(CS42448_Power_Control,0x11)) return false;
		assert(read(CS42448_Power_Control) == 0x11);
	#endif

	write(CS42448_Power_Control, 0xFF);
    delay(200);

    write(CS42448_Functional_Mode, default_config, sizeof(default_config));
    delay(100);

	write(CS42448_Power_Control, 0); // power up
    delay(500);

	#if 0
		LOG("cs42448 settings",0);
		LOG("0x01. Chip_ID            = 0x%02x",read(CS42448_Chip_ID));
		LOG("0x02. Power_Control      = 0x%02x",read(CS42448_Power_Control));
		LOG("0x03. Functional_Mode    = 0x%02x",read(CS42448_Functional_Mode));
		LOG("0x04. Interface_Formats  = 0x%02x",read(CS42448_Interface_Formats));
		LOG("0x05. ADC_Control        = 0x%02x",read(CS42448_ADC_Control));
		LOG("0x06. Transition_Control = 0x%02x",read(CS42448_Transition_Control));
		LOG("0x07. DAC_Channel_Mute   = 0x%02x",read(CS42448_DAC_Channel_Mute));
		LOG("0x10. DAC_Channel_Invert = 0x%02x",read(CS42448_DAC_Channel_Invert));
		LOG("0x17. ADC_Channel_Invert = 0x%02x",read(CS42448_ADC_Channel_Invert));
		LOG("0x18. Status_Control     = 0x%02x",read(CS42448_Status_Control));
		LOG("0x19. Status             = 0x%02x",read(CS42448_Status));
		LOG("0x1A. Status_Mask        = 0x%02x",read(CS42448_Status_Mask));
		LOG("0x1B. MUTEC_Pin_Control  = 0x%02x",read(CS42448_MUTEC_Pin_Control));
        delay(200);
	#endif

	volume(0);
}


//------------------------------
// supported methods
//------------------------------

uint32_t volumebyte(float level)
	// convert level to volume byte, section 6.9.1, page 50
{
	if (level >= 1.0) return 0;
	if (level <= 0.0000003981) return 128;
	uint32_t i = roundf(log10f(level) * -20.0);
	// LOG("volumeByte %0.2f==%d",level,i);
	return i;
}
int32_t inputlevelbyte(float level)
	// convert level to input gain, section 6.11.1, page 51
	// from +24 to -64 db
	//
	// 0x30+  0011 0000 +24 dB          decimal 48
	//        ··· ···
	//        0000 0000 0 dB            0 == 0 db = default
	//        1111 1111 -0.5 dB         -1
	//        1111 1110 -1 dB           -2
	//        ··· ···
	//        1000 0000 -64 dB	        -128
{
	// pauls algorithm
	//
	// if (level > 15.8489) return 48;
	// if (level < 0.00063095734) return -128;
	// return roundf(log10f(level) * 40.0);

	int32_t i = roundf(level * (128 + 1 + 48)) - 128;
	// LOG("inputlevelbyte %0.2f==%d",level,i);
	return i;
}



void AudioControlCS42448::volume(float level)
{
	u32 n = volumebyte(level);
	m_muteMask = n == 255 ? 0xff : 0;
	uint8_t data[9];
	data[0] = m_muteMask;
	for (int i=1; i < 9; i++)
	{
		data[i] = n;
	}
	// display_bytes("cs2448::volume()",data,9);
	write(CS42448_DAC_Channel_Mute, data, 9);
}


void AudioControlCS42448::volume(int channel, float level)
{
	u32 mask = (1 << channel);
	u32 n = volumebyte(level);
	m_muteMask = n == 255 ?
		m_muteMask | mask :
		m_muteMask & ~mask;
	write(CS42448_DAC_Channel_Mute, m_muteMask);
	write(CS42448_DAC_Channel_Mute+channel+1, n);
}


void AudioControlCS42448::inputLevel(float level)
{
	u32 n = inputlevelbyte(level);
	for (int i=0; i<6; i++)
		write(CS42448_AIN1_Volume_Control+i,n);

}
void AudioControlCS42448::inputLevel(int channel, float level)
{
	if (channel < 1 || channel > 6) return;
	u32 n = inputlevelbyte(level);
	write(CS42448_AIN1_Volume_Control+channel,n);
}



//------------------------------
// low level utilities
//------------------------------

void AudioControlCS42448::write(uint32_t address, uint32_t data)
{
    // printf("write_reg(0x%02x,0x%02x)\n",address,data);
	Wire.beginTransmission(I2C_ADDR);
	Wire.write(address);
	Wire.write(data);
	Wire.endTransmission();
    CTimer::Get()->usDelay(50);
}


void AudioControlCS42448::write(uint32_t address, const void *data, uint32_t len, bool auto_inc)
{
	Wire.beginTransmission(I2C_ADDR);

    if (auto_inc)
    {
        Wire.write(address | 0x80); // 0x80 = auto increment bit
    }
    else
    {
        Wire.write(address); // he incorrectly does not set increment bit
    }

	const uint8_t *p = (const uint8_t *)data;
	const uint8_t *end = p + len;
	while (p < end)
    {
		Wire.write(*p++);
	}
	Wire.endTransmission();
}



u8 AudioControlCS42448::read(u8 address)
	// read the value of a register
{
	u8 buf[2];
	buf[0] = 0;
	Wire.beginTransmission(I2C_ADDR);
	Wire.write(address);
	Wire.endTransmission();
	CTimer::Get()->usDelay(100);
	Wire.read(I2C_ADDR,buf,1);
	CTimer::Get()->usDelay(1200);
	return buf[0];
}


//------------------------------
// Octo specific GPIO
//------------------------------

#ifdef AUDIO_INJECTOR_OCTO

    void AudioControlCS42448::reset()
    {
        if (!octo_reset)
        {
            octo_reset = new CGPIOPin(OCTO_PIN_RESET,GPIOModeOutput);
            octo_mult[0] = new CGPIOPin(OCTO_PIN_MULT0,GPIOModeOutput);
            octo_mult[1] = new CGPIOPin(OCTO_PIN_MULT1,GPIOModeOutput);
            octo_mult[2] = new CGPIOPin(OCTO_PIN_MULT2,GPIOModeOutput);
            octo_mult[3] = new CGPIOPin(OCTO_PIN_MULT3,GPIOModeOutput);

            assert(octo_reset);
            assert(octo_mult[0]);
            assert(octo_mult[1]);
            assert(octo_mult[2]);
            assert(octo_mult[3]);
        }

        LOG("reset()",0);
        octo_reset->Write(1);
        delay(1000);
        octo_reset->Write(0);
        delay(200);
        octo_reset->Write(1);
    }


    void AudioControlCS42448::startClock()
    {
        u8 mult[4];
        memset(mult,0,4);
        LOG("setSamnpleRate(%d)",g_sample_rate);

		switch (g_sample_rate)
        {
            case 96000:
                mult[3] = 1;
            case 88200:
                mult[1] = 1;
                mult[2] = 1;
                break;
            case 48000:
                mult[3] = 1;
            case 44100:
                mult[2] = 1;
                break;
            case 32000:
                mult[3] = 1;
            case 29400:
                mult[0] = 1;
                mult[1] = 1;
                break;
            case 24000:
                mult[3] = 1;
            case 22050:
                mult[1] = 1;
                break;
            case 16000:
                mult[3] = 1;
            case 14700:
                mult[0] = 1;
                break;
            case 8000:
                mult[3] = 1;
                break;
            default:
                break;
        }

        for (u8 i=0; i<4; i++)
        {
            // printf("mult[%d]=%d\n",i,mult[i]);
            octo_mult[i]->Write(mult[i]);
        }
    }

#endif	// AUDIO_INJECTOR_OCTO
