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

    
#if  0      

    // This code more or less emulates exactly the i2c signals
    // and startup sequence sent by Raspian when starting an MP3
    // the first time.

    u8 raspian_default_regmap[] = {
        0xFF,   // Power Control 
        0xF0,   // Functional Mode 
        0x46,   // Interface Formats 
            // prh THIS DEFAULT IS WRONG
            // 0x46 == DAC_DIF_LEFTJ | ADC_DIF_TDM | AUX_I2S
            // The actual default is
            // 0x36 = DAC_DIF_TDM | ADC_DIF_TDM
        0x00,   // ADC Control & DAC De-Emphasis */
        0x10,   // Transition Control */
        0xFF,   // DAC Channel Mute
            // he sends out 0xff for this
            // although the actual default is
            // 0x00,   
        0x00,   // Volume Control AOUT1
        0x00,   // Volume Control AOUT2
        0x00,   // Volume Control AOUT3
        0x00,   // Volume Control AOUT4
        0x00,   // Volume Control AOUT5
        0x00,   // Volume Control AOUT6
        0x00,   // Volume Control AOUT7
        0x00,   // Volume Control AOUT8
        0x00,   // DAC Channel Invert
        0x00,   // Volume Control AIN1
        0x00,   // Volume Control AIN2
        0x00,   // Volume Control AIN3
        0x00,   // Volume Control AIN4
        0x00,   // Volume Control AIN5
        0x00,   // Volume Control AIN6
        0x00,   // ADC Channel Invert 
        0x00,   // Status Control
        0x00,   // Status Mask 
        0x00,   // MUTEC Pin Control 
    };

    u8 two_zeros[] = {0,0};

    bool AudioControlCS42448::raspianStartup()
    {
        LOG("raspianStartup()",0);
        
        // Raspian starts by sending the default regmap, incorrect
        // as it is, with power control and DAC mute set to 0xFF,
        // without setting the increment bit. By spec this is undefined.
        
        if (!write(CS42448_Power_Control,raspian_default_regmap,sizeof(raspian_default_regmap),false))
            return false;
        CTimer::Get()->usDelay(70);

        // Then it sends outputs two bytes of zeros for the status mask,
        // also without setting the increment bit
        
        if (!write(CS42448_Status_Mask, two_zeros, 2,false)) // 0x1a
            return false;
        delay(2);

        // then it sets the interface formats with a 50us delay

        if (!write(CS42448_Interface_Formats,               // 0x04
            INTFC_ADC_TDM | INTFC_DAC_TDM | INTFC_AUX_I2S)) // 0x76
            return false;
        CTimer::Get()->usDelay(50);
        
        // and the functional mode with a 2ms delay
        
        if (!write(CS42448_Functional_Mode,                 // 0x03
            FM_ADC_SPEED_AUTO_SLAVE | FM_DAC_SPEED_AUTO_SLAVE | FM_MCLK_4_TO_51_MHZ))  // 0xF8
           return false;
        delay(2);
        
        // then a bunch of 3 settings, starting with power down
        // all but not the chip
        
        if (!write(CS42448_Power_Control, 0xfe))    // 0x02
                return false; 
            
        // then another power E0, with a little extra delay
        
        if (!write(CS42448_Power_Control, 0xE0))    // 0x02
                return false;
        CTimer::Get()->usDelay(50);
        
        // followed by channel mute 0, a 200 us delay
        // and a repeat
        
        if (!write(CS42448_DAC_Channel_Mute, 0x00)) // 0x07
            return false;
        CTimer::Get()->usDelay(200);
        if (!write(CS42448_DAC_Channel_Mute, 0x00)) // 0x07
            return false;

        // then approximately 60ms later the clock is started

        delay(60);        
        octo_mult[2]->Write(1);
        // setSampleRate(44100);
        
        return true;
    }
#endif  // 0 = Raspian startp emulator


//------------------------------------------------------
// actual code
//------------------------------------------------------

static const uint8_t default_config[] =
{
#ifdef __circle__
    FM_ADC_SPEED_AUTO_SLAVE | FM_DAC_SPEED_AUTO_SLAVE | FM_MCLK_4_TO_51_MHZ,
        // CS42448_Functional_Mode = 0xF9 = slave mode, MCLK 51.2 MHz max
#else
    // slave does not work
    FM_ADC_SPEED_AUTO_SLAVE | FM_DAC_SPEED_AUTO_SLAVE | FM_MCLK_2_TO_25_MHZ,
        // CS42448_Functional_Mode = 0xF6 = slave mode, MCLK 25.6 MHz max
#endif
        
    INTFC_ADC_TDM | INTFC_DAC_TDM | INTFC_AUX_I2S, 
        // CS42448_Interface_Formats = T0x76 = DM mode + aux I2s
    ADCC_ADC3_SINGLE | ADCC_ADC2_SINGLE | ADCC_ADC1_SINGLE,
        // CS42448_ADC_Control_DAC_DeEmphasis = 0x1C = single ended ADC
    TRANS_ADC_SZC0 | TRANS_ADC_SZC1 | TRANS_DAC_SZC0 | TRANS_DAC_SZC1,
        // CS42448_Transition_Control = 0x63 = soft vol control

    #if 0
        0x00    // unmute all for easier testing
    #else
        0xff    // CS42448_DAC_Channel_Mute = 0xff = all outputs mute
    #endif
};



bool AudioControlCS42448::enable(void)
{
	LOG("enable()",0);

    #ifdef __circle__
        reset();
    #endif
    
	Wire.begin();

	#if __circle__
    
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
	#endif
	
	if (!write(CS42448_Power_Control, 0xFF)) return false; // power down
    delay(200);

    if (!write(CS42448_Functional_Mode, default_config, sizeof(default_config))) return false;
    delay(100);

	if (!write(CS42448_Power_Control, 0)) return false; // power up
    delay(500);
    
	#if 0   // def __circle__
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
    
    #ifdef __circle__
        // can't set the sample rate (start the clock) here.
        // dunno why, but it does not work
        //
        //      setSampleRate(44100);
        //
        // This must be called after the the DMA is setup, i.e.
        // after the AudioMemory() macro in the current implementation,
        // soon after the starts the bcm_pcm i2s device, or more accurately
        // in the bcm_pcm itself if BCM_KNOWS_OCTO
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
        assert(false);  // was not implemented on teensy
    #endif

	return true;
}


bool AudioControlCS42448::inputLevelInteger(int32_t n)
{
    #ifdef __circle__
        assert(false);  // was not implemented on teensy
    #endif


	return true;
}


bool AudioControlCS42448::inputLevelInteger(int chnnel, int32_t n)
{
    #ifdef __circle__
        assert(false);  // was not implemented on teensy
    #endif

	return true;
}


bool AudioControlCS42448::write(uint32_t address, uint32_t data)
{
    // printf("write_reg(0x%02x,0x%02x)\n",address,data);
	Wire.beginTransmission(i2c_addr);
	Wire.write(address);
	Wire.write(data);
	if (Wire.endTransmission() == 0) return true;
    CTimer::Get()->usDelay(50);
    
	return false;
}


bool AudioControlCS42448::write(uint32_t address, const void *data, uint32_t len, bool auto_inc)
{
	Wire.beginTransmission(i2c_addr);
    
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
        // printf("write_bulk(0x%02x,0x%02x)\n",address++,*p);
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
        octo_reset->Write(1);
        delay(1000);
        octo_reset->Write(0);
        delay(200);
        octo_reset->Write(1);
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
    

