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
#include "Wire.h"

#ifdef AUDIO_INJECTOR_OCTO   
    // __circle__ only code
    //
    // Note: for the Audio Injector Octo both the bcm and the codec
    // are set as slaves, and you wake up the fpga with reset() and
    // setSampleRate() for it to become the clock master.

    #include <circle/timer.h>
    #include <circle/gpiopin.h>
    #include <circle/logger.h>
    
    #define log_name "cm42448"

	#define OCTO_PIN_RESET	5
	#define OCTO_PIN_MULT0  27
	#define OCTO_PIN_MULT1  22
	#define OCTO_PIN_MULT2  23
	#define OCTO_PIN_MULT3  24
    
    CGPIOPin *octo_reset;
    CGPIOPin *octo_mult[4];
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


// 4.9 Recommended Power-Up Sequence
//    1. Hold RST low until the power supply and clocks are stable. In this state,
//       the control port is reset to its default settings and VQ will remain low.
//    2. Bring RST high. The device will initially be in a low power state with VQ
//       low. All features will default as described in the "Register Quick Reference"
//       on page 40.
//    3. Perform a write operation to the Power Control register ("Power Control
//       (Address 02h)" on page 43) to set bit 0 to a '1'b.  This will place the
//       device in a power down state.
//    4. Load the desired register settings while keeping the PDN bit set to '1'b.
//    5. Mute all DACs. Muting the DACs suppresses any noise associated with the
//       CODEC's first initialization after power is applied.
//    6. Set the PDN bit in the power control register to '0'b. VQ will ramp to
//       approximately VA/2 according to the Popguard specification in section
//       "Popguard" on page 29.
//    7. Following approximately 2000 LRCK cycles, the device is initialized and
//       ready for normal operation.
//    8. After the CODEC is initialized, wait ~90 LRCK cycles (~1.9 ms @48 kHz) and
//       then un-mute the DACs.
//    9. Normal operation begins.


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


static const uint8_t default_config[] = {
    // by a bug it started working
    // writing:
    //      functional mode 0x76
    //          FM_MCLK_2_TO_25_MHZ = 0x04 .. 2 is a reserved value
    //          FM_ADC_SPEED_AUTO_SLAVE  = 0x30
    //          FM_DAC_SPEED_DOUBLE = 0x40
    //
    //      interface       0x1c
    //          INTFC_ADC_ONELINE_1 = 0x04
    //          INTFC_DAC_RJUST16   = 0x18
    //      adc_control     0x63
    
#if 1
    // from bug:
    // FM_MCLK_2_TO_25_MHZ | FM_ADC_SPEED_AUTO_SLAVE | FM_DAC_SPEED_DOUBLE,
    // single or double sort of work, single sounds the best
    FM_MCLK_2_TO_25_MHZ | FM_ADC_SPEED_SINGLE | FM_DAC_SPEED_SINGLE,
    // FM_MCLK_2_TO_25_MHZ | FM_ADC_SPEED_DOUBLE | FM_DAC_SPEED_DOUBLE,
    // quad does not work:
    // FM_MCLK_2_TO_25_MHZ | FM_ADC_SPEED_QUAD   | FM_DAC_SPEED_QUAD,
#else
    // slave does not work
    FM_MCLK_2_TO_25_MHZ | FM_ADC_SPEED_AUTO_SLAVE | FM_DAC_SPEED_AUTO_SLAVE,
        // 0xF4, // CS42448_Functional_Mode = slave mode, MCLK 25.6 MHz max
#endif

    // i2s, rjust, maybe ljust interfaces produce results but
    // cannot be right
    //
    // TDM does not work.
    // That leaves oneline1 and 2
    // Oneline2 sounds better
    //
    // It's unreliable .. upon reboot sometimes it sounds better, or worse
    // but TDM never works
    
    INTFC_ADC_ONELINE_2 | INTFC_DAC_ONELINE_2 | INTFC_AUX_I2S,
    
    // old teensy setting:
    // INTFC_ADC_TDM | INTFC_DAC_TDM | INTFC_AUX_I2S, 
        // 0x76, // CS42448_Interface_Formats = TDM mode + aux I2s

    // the rest is unchanged from the original teensy settings
    
    ADCC_ADC3_SINGLE | ADCC_ADC2_SINGLE | ADCC_ADC1_SINGLE,
        // 0x1C, // CS42448_ADC_Control_DAC_DeEmphasis = single ended ADC

    TRANS_ADC_SZC0 | TRANS_ADC_SZC1 | TRANS_DAC_SZC0 | TRANS_DAC_SZC1,
        // 0x63, // CS42448_Transition_Control = soft vol control
        
    #ifdef __circle
        0x00
    #else
        // muting all the channels proves that volume control is not working
        0xff  // CS42448_DAC_Channel_Mute = all outputs mute
    #endif
};



bool AudioControlCS42448::enable(void)
{
	Wire.begin();
	
	#if 1
       // __circle__ sanity check code to make sure I am talking i2c
        // I get all the proper defaults except the chip revision appears
        // to be 0x04 instead of documented 0x01
		LOG("identifying chip",0);
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
        // test a read write operation
        if (!write(CS42448_Power_Control,0x11)) return false;
        assert(read(CS42448_Power_Control) == 0x11);
	#endif
	
	if (!write(CS42448_Power_Control, 0xFF)) return false; // power down
    delay(200);

    if (!write(CS42448_Functional_Mode, default_config, sizeof(default_config))) return false;
    delay(100);
    
	if (!write(CS42448_Power_Control, 0)) return false; // power up
    delay(500);
    
	#if 1   // def __circle__
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
    
	return true;
}


bool AudioControlCS42448::volumeInteger(uint32_t n)
{
	uint8_t data[9];
	data[0] = 0;
	for (int i=1; i < 9; i++) {
		data[i] = n;
	}
	return write(CS42448_DAC_Channel_Mute, data, 9);
}


bool AudioControlCS42448::volumeInteger(int channel, uint32_t n)
{
    #ifdef __circle__
        // was not implemented on teensy
    #endif

	return true;
}


bool AudioControlCS42448::inputLevelInteger(int32_t n)
{
    #ifdef __circle__
        // was not implemented on teensy
    #endif


	return true;
}


bool AudioControlCS42448::inputLevelInteger(int chnnel, int32_t n)
{
    #ifdef __circle__
        // was not implemented on teensy
    #endif

	return true;
}


bool AudioControlCS42448::write(uint32_t address, uint32_t data)
{
    printf("write_reg(0x%02x,0x%02x)\n",address,data);
	Wire.beginTransmission(i2c_addr);
	Wire.write(address);
	Wire.write(data);
	if (Wire.endTransmission() == 0) return true;
	return false;
}


bool AudioControlCS42448::write(uint32_t address, const void *data, uint32_t len)
{
	Wire.beginTransmission(i2c_addr);
	Wire.write(address | 0x80);
	const uint8_t *p = (const uint8_t *)data;
	const uint8_t *end = p + len;
	while (p < end)
    {
        printf("write_bulk(0x%02x,0x%02x)\n",address++,*p);
		Wire.write(*p++);
	}
	if (Wire.endTransmission() == 0) return true;
	return false;
}


#ifdef __circle__
	u8 AudioControlCS42448::read(u8 address)
		// read the value of a register
	{
		u8 buf[2];
        buf[0] = 0;
        Wire.beginTransmission(i2c_addr);
        Wire.write(address);
        Wire.endTransmission();
        CTimer::Get()->usDelay(100);
		Wire.read(i2c_addr,buf,1);
        CTimer::Get()->usDelay(1200);
		return buf[0];
	}
#endif


#ifdef AUDIO_INJECTOR_OCTO
    // __circle__ only at this time
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
        // setSampleRate(0);
        // delay(200);
        octo_reset->Write(1);
        delay(500);
        octo_reset->Write(0);
        delay(500);
        octo_reset->Write(1);
        delay(500);
    }
    
    
    void AudioControlCS42448::setSampleRate(u32 rate)
    {
        u8 mult[4];
        memset(mult,0,4);
        LOG("setSamnpleRate(%d)",rate);
    
		switch (rate)
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
#endif
    

