//-------------------------------------------------------------------------
// control_sgtl5000.cpp
//-------------------------------------------------------------------------
// Audio Library for Teensy 3.X
// Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
// please see LICENSE.TXT.

#include <Arduino.h>
#include "control_sgtl5000.h"
#include <assert.h>
#include <audio/bcm_pcm.h>
#include <circle/logger.h>
#include "khrn_int_math.h"
#include "Wire.h"
    
#define log_name "sgtl5000"

#define DEBUG_API		1
#define DEBUG_AUTO		1


#define USE_8MHZ_CLOCK	1
#define PIN_MCLK     	4
#define CLOCK_RATE   	500000000
#define SAMPLE_RATE  	44100

#define DUMP_CCS		0

#if DEBUG_API
	#define DBG_API(f,...)     CLogger::Get()->Write(log_name,LogDebug,f,__VA_ARGS__)
#else
	#define DBG_API(f,...)
#endif

#if DEBUG_AUTO
	#define DBG_AUTO(f,...)     CLogger::Get()->Write(log_name,LogDebug,f,__VA_ARGS__)
#else
	#define DBG_AUTO(f,...)
#endif


//-------------------------------------
// sgtl5000 register descriptions
//-------------------------------------

#define CHIP_ID				0x0000
// 15:8 PARTID		0xA0 - 8 bit identifier for SGTL5000
// 7:0  REVID		0x00 - revision number for SGTL5000.

#define CHIP_DIG_POWER			0x0002
// 6	ADC_POWERUP	1=Enable, 0=disable the ADC block, both digital & analog, 
// 5	DAC_POWERUP	1=Enable, 0=disable the DAC block, both analog and digital
// 4	DAP_POWERUP	1=Enable, 0=disable the DAP block
// 1	I2S_OUT_POWERUP	1=Enable, 0=disable the I2S data output
// 0	I2S_IN_POWERUP	1=Enable, 0=disable the I2S data input

#define CHIP_CLK_CTRL			0x0004
// 5:4	RATE_MODE	Sets the sample rate mode. MCLK_FREQ is still specified
//			relative to the rate in SYS_FS
//				0x0 = SYS_FS specifies the rate
//				0x1 = Rate is 1/2 of the SYS_FS rate
//				0x2 = Rate is 1/4 of the SYS_FS rate
//				0x3 = Rate is 1/6 of the SYS_FS rate
// 3:2	SYS_FS		Sets the internal system sample rate (default=2)
//				0x0 = 32 kHz
//				0x1 = 44.1 kHz
//				0x2 = 48 kHz
//				0x3 = 96 kHz
// 1:0	MCLK_FREQ	Identifies incoming SYS_MCLK frequency and if the PLL should be used
//				0x0 = 256*Fs
//				0x1 = 384*Fs
//				0x2 = 512*Fs
//				0x3 = Use PLL
//				The 0x3 (Use PLL) setting must be used if the SYS_MCLK is not
//				a standard multiple of Fs (256, 384, or 512). This setting can
//				also be used if SYS_MCLK is a standard multiple of Fs.
//				Before this field is set to 0x3 (Use PLL), the PLL must be
//				powered up by setting CHIP_ANA_POWER->PLL_POWERUP and
//				CHIP_ANA_POWER->VCOAMP_POWERUP.  Also, the PLL dividers must
//				be calculated based on the external MCLK rate and
//				CHIP_PLL_CTRL register must be set (see CHIP_PLL_CTRL register
//				description details on how to calculate the divisors).

#define CHIP_I2S_CTRL			0x0006
// 8	SCLKFREQ	Sets frequency of I2S_SCLK when in master mode (MS=1). When in slave
//			mode (MS=0), this field must be set appropriately to match SCLK input
//			rate.
//				0x0 = 64Fs
//				0x1 = 32Fs - Not supported for RJ mode (I2S_MODE = 1)
// 7	MS		Configures master or slave of I2S_LRCLK and I2S_SCLK.
//				0x0 = Slave: I2S_LRCLK an I2S_SCLK are inputs
//				0x1 = Master: I2S_LRCLK and I2S_SCLK are outputs
//				NOTE: If the PLL is used (CHIP_CLK_CTRL->MCLK_FREQ==0x3),
//				the SGTL5000 must be a master of the I2S port (MS==1)
// 6	SCLK_INV	Sets the edge that data (input and output) is clocked in on for I2S_SCLK
//				0x0 = data is valid on rising edge of I2S_SCLK
//				0x1 = data is valid on falling edge of I2S_SCLK
// 5:4	DLEN		I2S data length (default=1)
//				0x0 = 32 bits (only valid when SCLKFREQ=0),
//					not valid for Right Justified Mode
//				0x1 = 24 bits (only valid when SCLKFREQ=0)
//				0x2 = 20 bits
//				0x3 = 16 bits
// 3:2	I2S_MODE	Sets the mode for the I2S port
//				0x0 = I2S mode or Left Justified (Use LRALIGN to select)
//				0x1 = Right Justified Mode
//				0x2 = PCM Format A/B
//				0x3 = RESERVED
// 1	LRALIGN		I2S_LRCLK Alignment to data word. Not used for Right Justified mode
//				0x0 = Data word starts 1 I2S_SCLK delay after I2S_LRCLK
//					transition (I2S format, PCM format A)
//				0x1 = Data word starts after I2S_LRCLK transition (left
//					justified format, PCM format B)
// 0	LRPOL		I2S_LRCLK Polarity when data is presented.
//				0x0 = I2S_LRCLK = 0 - Left, 1 - Right
//				1x0 = I2S_LRCLK = 0 - Right, 1 - Left
//				The left subframe should be presented first regardless of
//				the setting of LRPOL.

#define CHIP_SSS_CTRL			0x000A
// 14	DAP_MIX_LRSWAP	DAP Mixer Input Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the DAP MIXER Input are swapped.
// 13	DAP_LRSWAP	DAP Mixer Input Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the DAP Input are swapped
// 12	DAC_LRSWAP	DAC Input Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the DAC are swapped
// 10	I2S_LRSWAP	I2S_DOUT Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the I2S_DOUT are swapped
// 9:8	DAP_MIX_SELECT	Select data source for DAP mixer
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = Reserved
// 7:6	DAP_SELECT	Select data source for DAP
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = Reserved
// 5:4	DAC_SELECT	Select data source for DAC (default=1)
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = DAP
// 1:0	I2S_SELECT	Select data source for I2S_DOUT
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = DAP

#define CHIP_ADCDAC_CTRL		0x000E
// 13	VOL_BUSY_DAC_RIGHT Volume Busy DAC Right
//				0x0 = Ready
//				0x1 = Busy - This indicates the channel has not reached its
//					programmed volume/mute level
// 12	VOL_BUSY_DAC_LEFT  Volume Busy DAC Left
//				0x0 = Ready
//				0x1 = Busy - This indicates the channel has not reached its
//					programmed volume/mute level
// 9	VOL_RAMP_EN	Volume Ramp Enable (default=1)
//				0x0 = Disables volume ramp. New volume settings take immediate
//					effect without a ramp
//				0x1 = Enables volume ramp
//				This field affects DAC_VOL. The volume ramp effects both
//				volume settings and mute When set to 1 a soft mute is enabled.
// 8	VOL_EXPO_RAMP	Exponential Volume Ramp Enable
//				0x0 = Linear ramp over top 4 volume octaves
//				0x1 = Exponential ramp over full volume range
//				This bit only takes effect if VOL_RAMP_EN is 1.
// 3	DAC_MUTE_RIGHT	DAC Right Mute (default=1)
//				0x0 = Unmute
//				0x1 = Muted
//				If VOL_RAMP_EN = 1, this is a soft mute.
// 2	DAC_MUTE_LEFT	DAC Left Mute (default=1)
//				0x0 = Unmute
//				0x1 = Muted
//				If VOL_RAMP_EN = 1, this is a soft mute.
// 1	ADC_HPF_FREEZE	ADC High Pass Filter Freeze
//				0x0 = Normal operation
//				0x1 = Freeze the ADC high-pass filter offset register.  The
//				offset continues to be subtracted from the ADC data stream.
// 0	ADC_HPF_BYPASS	ADC High Pass Filter Bypass
//				0x0 = Normal operation
//				0x1 = Bypassed and offset not updated

#define CHIP_DAC_VOL			0x0010
// 15:8	DAC_VOL_RIGHT	DAC Right Channel Volume.  Set the Right channel DAC volume
//			with 0.5017 dB steps from 0 to -90 dB
//				0x3B and less = Reserved
//				0x3C = 0 dB
//				0x3D = -0.5 dB
//				0xF0 = -90 dB
//				0xFC and greater = Muted
//				If VOL_RAMP_EN = 1, there is an automatic ramp to the
//				new volume setting.
// 7:0	DAC_VOL_LEFT	DAC Left Channel Volume.  Set the Left channel DAC volume
//			with 0.5017 dB steps from 0 to -90 dB
//				0x3B and less = Reserved
//				0x3C = 0 dB
//				0x3D = -0.5 dB
//				0xF0 = -90 dB
//				0xFC and greater = Muted
//				If VOL_RAMP_EN = 1, there is an automatic ramp to the
//				new volume setting.

#define CHIP_PAD_STRENGTH		0x0014
// 9:8	I2S_LRCLK	I2S LRCLK Pad Drive Strength (default=1)
//				Sets drive strength for output pads per the table below.
//				 VDDIO    1.8 V     2.5 V     3.3 V
//				0x0 = Disable
//				0x1 =     1.66 mA   2.87 mA   4.02 mA
//				0x2 =     3.33 mA   5.74 mA   8.03 mA
//				0x3 =     4.99 mA   8.61 mA   12.05 mA
// 7:6	I2S_SCLK	I2S SCLK Pad Drive Strength (default=1)
// 5:4	I2S_DOUT	I2S DOUT Pad Drive Strength (default=1)
// 3:2	CTRL_DATA	I2C DATA Pad Drive Strength (default=3)
// 1:0	CTRL_CLK	I2C CLK Pad Drive Strength (default=3)
//				(all use same table as I2S_LRCLK)

#define CHIP_ANA_ADC_CTRL		0x0020
// 8	ADC_VOL_M6DB	ADC Volume Range Reduction
//				This bit shifts both right and left analog ADC volume
//				range down by 6.0 dB.
//				0x0 = No change in ADC range
//				0x1 = ADC range reduced by 6.0 dB
// 7:4	ADC_VOL_RIGHT	ADC Right Channel Volume
//				Right channel analog ADC volume control in 1.5 dB steps.
//				0x0 = 0 dB
//				0x1 = +1.5 dB
//				...
//				0xF = +22.5 dB
//				This range is -6.0 dB to +16.5 dB if ADC_VOL_M6DB is set to 1.
// 3:0	ADC_VOL_LEFT	ADC Left Channel Volume
//				(same scale as ADC_VOL_RIGHT)

#define CHIP_ANA_HP_CTRL		0x0022
// 14:8	HP_VOL_RIGHT	Headphone Right Channel Volume  (default 0x18)
//				Right channel headphone volume control with 0.5 dB steps.
//				0x00 = +12 dB
//				0x01 = +11.5 dB
//				0x18 = 0 dB
//				...
//				0x7F = -51.5 dB
// 6:0	HP_VOL_LEFT	Headphone Left Channel Volume  (default 0x18)
//				(same scale as HP_VOL_RIGHT)

#define CHIP_ANA_CTRL			0x0024
// 8	MUTE_LO		LINEOUT Mute, 0 = Unmute, 1 = Mute  (default 1)
// 6	SELECT_HP	Select the headphone input, 0 = DAC, 1 = LINEIN
// 5	EN_ZCD_HP	Enable the headphone zero cross detector (ZCD)
//				0x0 = HP ZCD disabled
//				0x1 = HP ZCD enabled
// 4	MUTE_HP		Mute the headphone outputs, 0 = Unmute, 1 = Mute (default)
// 2	SELECT_ADC	Select the ADC input, 0 = Microphone, 1 = LINEIN
// 1	EN_ZCD_ADC	Enable the ADC analog zero cross detector (ZCD)
//				0x0 = ADC ZCD disabled
//				0x1 = ADC ZCD enabled
// 0	MUTE_ADC	Mute the ADC analog volume, 0 = Unmute, 1 = Mute (default)

#define CHIP_LINREG_CTRL		0x0026
// 6	VDDC_MAN_ASSN	Determines chargepump source when VDDC_ASSN_OVRD is set.
//				0x0 = VDDA
//				0x1 = VDDIO
// 5	VDDC_ASSN_OVRD	Charge pump Source Assignment Override
//				0x0 = Charge pump source is automatically assigned based
//					on higher of VDDA and VDDIO
//				0x1 = the source of charge pump is manually assigned by
//					VDDC_MAN_ASSN If VDDIO and VDDA are both the same
//					and greater than 3.1 V, VDDC_ASSN_OVRD and
//					VDDC_MAN_ASSN should be used to manually assign
//					VDDIO as the source for charge pump.
// 3:0	D_PROGRAMMING	Sets the VDDD linear regulator output voltage in 50 mV steps.
//			Must clear the LINREG_SIMPLE_POWERUP and STARTUP_POWERUP bits
//			in the 0x0030 (CHIP_ANA_POWER) register after power-up, for
//			this setting to produce the proper VDDD voltage.
//				0x0 = 1.60
//				0xF = 0.85

#define CHIP_REF_CTRL			0x0028 // bandgap reference bias voltage and currents
// 8:4	VAG_VAL		Analog Ground Voltage Control
//				These bits control the analog ground voltage in 25 mV steps.
//				This should usually be set to VDDA/2 or lower for best
//				performance (maximum output swing at minimum THD). This VAG
//				reference is also used for the DAC and ADC voltage reference.
//				So changing this voltage scales the output swing of the DAC
//				and the output signal of the ADC.
//				0x00 = 0.800 V
//				0x1F = 1.575 V
// 3:1	BIAS_CTRL	Bias control
//				These bits adjust the bias currents for all of the analog
//				blocks. By lowering the bias current a lower quiescent power
//				is achieved. It should be noted that this mode can affect
//				performance by 3-4 dB.
//				0x0 = Nominal
//				0x1-0x3=+12.5%
//				0x4=-12.5%
//				0x5=-25%
//				0x6=-37.5%
//				0x7=-50%
// 0	SMALL_POP	VAG Ramp Control
//				Setting this bit slows down the VAG ramp from ~200 to ~400 ms
//				to reduce the startup pop, but increases the turn on/off time.
//				0x0 = Normal VAG ramp
//				0x1 = Slow down VAG ramp

#define CHIP_MIC_CTRL			0x002A // microphone gain & internal microphone bias
// 9:8	BIAS_RESISTOR	MIC Bias Output Impedance Adjustment
//				Controls an adjustable output impedance for the microphone bias.
//				If this is set to zero the micbias block is powered off and
//				the output is highZ.
//				0x0 = Powered off
//				0x1 = 2.0 kohm
//				0x2 = 4.0 kohm
//				0x3 = 8.0 kohm
// 6:4	BIAS_VOLT	MIC Bias Voltage Adjustment
//				Controls an adjustable bias voltage for the microphone bias
//				amp in 250 mV steps. This bias voltage setting should be no
//				more than VDDA-200 mV for adequate power supply rejection.
//				0x0 = 1.25 V
//				...
//				0x7 = 3.00 V
// 1:0	GAIN		MIC Amplifier Gain
//				Sets the microphone amplifier gain. At 0 dB setting the THD
//				can be slightly higher than other paths- typically around
//				~65 dB. At other gain settings the THD are better.
//				0x0 = 0 dB
//				0x1 = +20 dB
//				0x2 = +30 dB
//				0x3 = +40 dB

#define CHIP_LINE_OUT_CTRL		0x002C
// 11:8	OUT_CURRENT	Controls the output bias current for the LINEOUT amplifiers.  The
//			nominal recommended setting for a 10 kohm load with 1.0 nF load cap
//			is 0x3. There are only 5 valid settings.
//				0x0=0.18 mA
//				0x1=0.27 mA
//				0x3=0.36 mA
//				0x7=0.45 mA
//				0xF=0.54 mA
// 5:0	LO_VAGCNTRL	LINEOUT Amplifier Analog Ground Voltage
//				Controls the analog ground voltage for the LINEOUT amplifiers
//				in 25 mV steps. This should usually be set to VDDIO/2.
//				0x00 = 0.800 V
//				...
//				0x1F = 1.575 V
//				...
//				0x23 = 1.675 V
//				0x24-0x3F are invalid

#define CHIP_LINE_OUT_VOL		0x002E
// 12:8	LO_VOL_RIGHT	LINEOUT Right Channel Volume (default=4)
//				Controls the right channel LINEOUT volume in 0.5 dB steps.
//				Higher codes have more attenuation.
// 4:0	LO_VOL_LEFT	LINEOUT Left Channel Output Level (default=4)
//				Used to normalize the output level of the left line output
//				to full scale based on the values used to set
//				LINE_OUT_CTRL->LO_VAGCNTRL and CHIP_REF_CTRL->VAG_VAL.
//				In general this field should be set to:
//				40*log((VAG_VAL)/(LO_VAGCNTRL)) + 15
//				Suggested values based on typical VDDIO and VDDA voltages.
//					VDDA  VAG_VAL VDDIO  LO_VAGCNTRL LO_VOL_*
//					1.8 V    0.9   3.3 V     1.55      0x06
//					1.8 V    0.9   1.8 V      0.9      0x0F
//					3.3 V   1.55   1.8 V      0.9      0x19
//					3.3 V   1.55   3.3 V     1.55      0x0F
//				After setting to the nominal voltage, this field can be used
//				to adjust the output level in +/-0.5 dB increments by using
//				values higher or lower than the nominal setting.

#define CHIP_ANA_POWER			0x0030 // power down controls for the analog blocks.
		// The only other power-down controls are BIAS_RESISTOR in the MIC_CTRL register
		//  and the EN_ZCD control bits in ANA_CTRL.
// 14	DAC_MONO	While DAC_POWERUP is set, this allows the DAC to be put into left only
//				mono operation for power savings. 0=mono, 1=stereo (default)
// 13	LINREG_SIMPLE_POWERUP  Power up the simple (low power) digital supply regulator.
//				After reset, this bit can be cleared IF VDDD is driven
//				externally OR the primary digital linreg is enabled with
//				LINREG_D_POWERUP
// 12	STARTUP_POWERUP	Power up the circuitry needed during the power up ramp and reset.
//				After reset this bit can be cleared if VDDD is coming from
//				an external source.
// 11	VDDC_CHRGPMP_POWERUP Power up the VDDC charge pump block. If neither VDDA or VDDIO
//				is 3.0 V or larger this bit should be cleared before analog
//				blocks are powered up.
// 10	PLL_POWERUP	PLL Power Up, 0 = Power down, 1 = Power up
//				When cleared, the PLL is turned off. This must be set before
//				CHIP_CLK_CTRL->MCLK_FREQ is programmed to 0x3. The
//				CHIP_PLL_CTRL register must be configured correctly before
//				setting this bit.
// 9	LINREG_D_POWERUP Power up the primary VDDD linear regulator, 0 = Power down, 1 = Power up
// 8	VCOAMP_POWERUP	Power up the PLL VCO amplifier, 0 = Power down, 1 = Power up
// 7	VAG_POWERUP	Power up the VAG reference buffer.
//				Setting this bit starts the power up ramp for the headphone
//				and LINEOUT. The headphone (and/or LINEOUT) powerup should
//				be set BEFORE clearing this bit. When this bit is cleared
//				the power-down ramp is started. The headphone (and/or LINEOUT)
//				powerup should stay set until the VAG is fully ramped down
//				(200 to 400 ms after clearing this bit).
//				0x0 = Power down, 0x1 = Power up
// 6	ADC_MONO	While ADC_POWERUP is set, this allows the ADC to be put into left only
//				mono operation for power savings. This mode is useful when
//				only using the microphone input.
//				0x0 = Mono (left only), 0x1 = Stereo
// 5	REFTOP_POWERUP	Power up the reference bias currents
//				0x0 = Power down, 0x1 = Power up
//				This bit can be cleared when the part is a sleep state
//				to minimize analog power.
// 4	HEADPHONE_POWERUP Power up the headphone amplifiers
//				0x0 = Power down, 0x1 = Power up
// 3	DAC_POWERUP	Power up the DACs
//				0x0 = Power down, 0x1 = Power up
// 2	CAPLESS_HEADPHONE_POWERUP Power up the capless headphone mode
//				0x0 = Power down, 0x1 = Power up
// 1	ADC_POWERUP	Power up the ADCs
//				0x0 = Power down, 0x1 = Power up
// 0	LINEOUT_POWERUP	Power up the LINEOUT amplifiers
//				0x0 = Power down, 0x1 = Power up

#define CHIP_PLL_CTRL			0x0032
// 15:11 INT_DIVISOR
// 10:0 FRAC_DIVISOR

#define CHIP_CLK_TOP_CTRL		0x0034
// 11	ENABLE_INT_OSC	Setting this bit enables an internal oscillator to be used for the
//				zero cross detectors, the short detect recovery, and the
//				charge pump. This allows the I2S clock to be shut off while
//				still operating an analog signal path. This bit can be kept
//				on when the I2S clock is enabled, but the I2S clock is more
//				accurate so it is preferred to clear this bit when I2S is present.
// 3	INPUT_FREQ_DIV2	SYS_MCLK divider before PLL input
//				0x0 = pass through
//				0x1 = SYS_MCLK is divided by 2 before entering PLL
//				This must be set when the input clock is above 17 Mhz. This
//				has no effect when the PLL is powered down.

#define CHIP_ANA_STATUS			0x0036
// 9	LRSHORT_STS	This bit is high whenever a short is detected on the left or right
//				channel headphone drivers.
// 8	CSHORT_STS	This bit is high whenever a short is detected on the capless headphone
//				common/center channel driver.
// 4	PLL_IS_LOCKED	This bit goes high after the PLL is locked.

#define CHIP_ANA_TEST1			0x0038 //  intended only for debug.
#define CHIP_ANA_TEST2			0x003A //  intended only for debug.

#define CHIP_SHORT_CTRL			0x003C
// 14:12 LVLADJR	Right channel headphone	short detector in 25 mA steps.
//				0x3=25 mA
//				0x2=50 mA
//				0x1=75 mA
//				0x0=100 mA
//				0x4=125 mA
//				0x5=150 mA
//				0x6=175 mA
//				0x7=200 mA
//				This trip point can vary by ~30% over process so leave plenty
//				of guard band to avoid false trips.  This short detect trip
//				point is also effected by the bias current adjustments made
//				by CHIP_REF_CTRL->BIAS_CTRL and by CHIP_ANA_TEST1->HP_IALL_ADJ.
// 10:8	LVLADJL		Left channel headphone short detector in 25 mA steps.
//				(same scale as LVLADJR)
// 6:4	LVLADJC		Capless headphone center channel short detector in 50 mA steps.
//				0x3=50 mA
//				0x2=100 mA
//				0x1=150 mA
//				0x0=200 mA
//				0x4=250 mA
//				0x5=300 mA
//				0x6=350 mA
//				0x7=400 mA
// 3:2	MODE_LR		Behavior of left/right short detection
//				0x0 = Disable short detector, reset short detect latch,
//					software view non-latched short signal
//				0x1 = Enable short detector and reset the latch at timeout
//					(every ~50 ms)
//				0x2 = This mode is not used/invalid
//				0x3 = Enable short detector with only manual reset (have
//					to return to 0x0 to reset the latch)
// 1:0	MODE_CM		Behavior of capless headphone central short detection
//				(same settings as MODE_LR)

#define DAP_CONTROL			        0x0100
#define DAP_PEQ				        0x0102
#define DAP_BASS_ENHANCE		    0x0104
#define DAP_BASS_ENHANCE_CTRL	    0x0106
#define DAP_AUDIO_EQ			    0x0108
#define DAP_SGTL_SURROUND		    0x010A
#define DAP_FILTER_COEF_ACCESS	    0x010C
#define DAP_COEF_WR_B0_MSB		    0x010E
#define DAP_COEF_WR_B0_LSB		    0x0110
#define DAP_AUDIO_EQ_BASS_BAND0	    0x0116 // 115 Hz
#define DAP_AUDIO_EQ_BAND1		    0x0118 // 330 Hz
#define DAP_AUDIO_EQ_BAND2		    0x011A // 990 Hz
#define DAP_AUDIO_EQ_BAND3		    0x011C // 3000 Hz
#define DAP_AUDIO_EQ_TREBLE_BAND4	0x011E // 9900 Hz
#define DAP_MAIN_CHAN			    0x0120
#define DAP_MIX_CHAN			    0x0122
#define DAP_AVC_CTRL			    0x0124
#define DAP_AVC_THRESHOLD		    0x0126
#define DAP_AVC_ATTACK			    0x0128
#define DAP_AVC_DECAY			    0x012A
#define DAP_COEF_WR_B1_MSB		    0x012C
#define DAP_COEF_WR_B1_LSB		    0x012E
#define DAP_COEF_WR_B2_MSB		    0x0130
#define DAP_COEF_WR_B2_LSB		    0x0132
#define DAP_COEF_WR_A1_MSB		    0x0134
#define DAP_COEF_WR_A1_LSB		    0x0136
#define DAP_COEF_WR_A2_MSB		    0x0138
#define DAP_COEF_WR_A2_LSB		    0x013A


// peronal notes on where registers are used
//
// CHIP_ID						0x0000
// CHIP_DIG_POWER				0x0002		enable
// CHIP_CLK_CTRL				0x0004		enable
// CHIP_I2S_CTRL				0x0006		enable
// gap 0x0008
// CHIP_SSS_CTRL				0x000A		enable		dapEnable()
// gap 0x000C
// CHIP_ADCDAC_CTRL				0x000E		enable		dacVolumeRamp()
// CHIP_DAC_VOL					0x0010		enable		dacVolume()
// gap 0x0012
// CHIP_PAD_STRENGTH			0x0014
// gap 0x0016
// gap 0x0018
// gap 0x001A
// gap 0x001C
// gpp 0x001E
// CHIP_ANA_ADC_CTRL			0x0020		enable		lineInLevel(), dacVolume(muting), adcHighPassFilter()
// CHIP_ANA_HP_CTRL				0x0022		enable		headphoneVolume()
// CHIP_ANA_CTRL				0x0024		enable		inputSelect(), headphoneSelect(), muteHeadphone(), muteLineOut()
// CHIP_LINREG_CTRL				0x0026
// CHIP_REF_CTRL				0x0028		enable
// CHIP_MIC_CTRL				0x002A					micGain()
// CHIP_LINE_OUT_CTRL			0x002C		enable
// CHIP_LINE_OUT_VOL			0x002E		enable		lineOutLevel()
// CHIP_ANA_POWER				0x0030		enable
// CHIP_PLL_CTRL				0x0032		enable if extMCLK
// CHIP_CLK_TOP_CTRL			0x0034		enable if extMCLK
// CHIP_ANA_STATUS				0x0036
// CHIP_ANA_TEST1				0x0038
// CHIP_ANA_TEST2				0x003A
// CHIP_SHORT_CTRL				0x003C		enable
// gap to 0x100
// DAP_CONTROL					0x0100					dapEnable()
// DAP_PEQ						0x0102
// DAP_BASS_ENHANCE				0x0104					enhanceBass(?), enhanceBassEnable()
// DAP_BASS_ENHANCE_CTRL		0x0106
// DAP_AUDIO_EQ					0x0108					eqSelect()
// DAP_SGTL_SURROUND			0x010A					surroundSoundEnable(), surroundSound()
// DAP_FILTER_COEF_ACCESS		0x010C
// DAP_COEF_WR_B0_MSB			0x010E
// DAP_COEF_WR_B0_LSB			0x0110
// gap 0x112
// gap 0x114
// DAP_AUDIO_EQ_BASS_BAND0		0x0116 // 115 Hz		eqBands(tone)
// DAP_AUDIO_EQ_BAND1			0x0118 // 330 Hz		eqBands()
// DAP_AUDIO_EQ_BAND2			0x011A // 990 Hz		eqBands()
// DAP_AUDIO_EQ_BAND3			0x011C // 3000 Hz		eqBands()
// DAP_AUDIO_EQ_TREBLE_BAND4	0x011E // 9900 Hz		eqBands(tone)
// DAP_MAIN_CHAN				0x0120
// DAP_MIX_CHAN					0x0122
// DAP_AVC_CTRL					0x0124
// DAP_AVC_THRESHOLD			0x0126
// DAP_AVC_ATTACK				0x0128
// DAP_AVC_DECAY				0x012A
// DAP_COEF_WR_B1_MSB			0x012C
// DAP_COEF_WR_B1_LSB			0x012E
// DAP_COEF_WR_B2_MSB			0x0130
// DAP_COEF_WR_B2_LSB			0x0132
// DAP_COEF_WR_A1_MSB			0x0134
// DAP_COEF_WR_A1_LSB			0x0136
// DAP_COEF_WR_A2_MSB			0x0138
// DAP_COEF_WR_A2_LSB			0x013A
// last register byte			0x013B

// I have not dealt with AVC, or the Pararmetric EQ, and the DAP_MIXER
// is not used in this implmentation


//-------------------------------------------------
// master device (for rPi to be the slave
//-------------------------------------------------
// The rPi MUST be a slave to an sgtl5000, as it cannot provide 3 clocks.
// Even so, the rPi must generate MCLK, as the sgtl5000 has no crystal.
// So, we generate MCLK on GPIO4, send it to the teensy audio shield,
// which then generates BCLK and MCLK and sends them back to us.



AudioControlSGTL5000::AudioControlSGTL5000() :
   	AudioCodec(0,NULL),
    m_i2c_addr(0x0A),
    m_MPIN(PIN_MCLK,GPIOModeAlternateFunction0),
    m_MCLK(GPIOClock0,GPIOClockSourcePLLD)
{
    bcm_pcm.static_init(
        true,			// bcm_pcm is slave device
        SAMPLE_RATE,    // sample_rate
        16,             // sample_size
        2,              // num_channels
        32,             // channel_width
        1,1,            // channel offsets
        1,				// invert BCLK
        0,              // don't invert FCLK
        0);             // no callback to start the clock

}                       





void AudioControlSGTL5000::start()
{
    LOG("start()",0);

	#if USE_8MHZ_CLOCK
		// then we also have to use the SGTL5000 pll capabilities

		LOG("starting 8Mhz MCLK",0);

		// get 2.75v p-p at at 8.06 mhz with simple div1 of 62
		//		div1 = 500,000,000 / 8,000,000 = 62.5
		//      modi = 4,000,000
		//      freq/2 = 4,000,000
		//		divf = (4M * 4096 + 4N) / 8M = 2048.5
		//			 = (4097 * 4M) / 8M = 2048

		m_MCLK.Start(62, 2048, 3);
			// need the third nMash parameter
			// and might as well use '3'

	#else
		LOG("starting MCLK",0);
		// The problem was that this 11.28 mhz clock only presents
		// about 1.3-1.65v p-p and I think that doesn't drive the
		// SGTL500 enough.

		u32 freq = SAMPLE_RATE * 256;           // 11289600
		u32 divi = CLOCK_RATE / freq;           // 44.253
		u32 modi = CLOCK_RATE % freq;           // 3257600
			// modi * 4096 =   13,343,129,600 = 0x3 1B500000	overflows 32 bits
			// freq/2 = 		    5,644,800 = 0x  00562200
			// sum = 		   13,348,774,400 = 0x3 1BA62200
			// div =		   1182	as commented below
		u32 divf = (modi*4096 + freq/2) / freq; // 1182
		// LOG("divf=%d should equal 1182\n",divf);
		// the above math overflows 32 bits
		divf = 1182;

		m_MCLK.Start(divi, divf, 1);
	#endif

    delay(500);
    
	// initialize members

	m_hp_muted = true;
	m_lineout_muted = true;
	memset(m_band_value,0x2f,5);
	memset(m_band_target,0x2f,5);
	m_in_automation	= 0;

	//------------------------
	// initialize sgtl5000
	//------------------------

    Wire.begin();
    
    LOG("chip ID=0x%04x",read(CHIP_ID));

	#if DUMP_CCS
		dumpCCValues("at top of start()");
	#endif

	uint16_t i2s_ctrl = read(CHIP_I2S_CTRL);

	LOG("i2s_ctrl=0x%04x",i2s_ctrl);
		// reset default = 0x10
		// anything else indicates soft-reboot
		// so we bail and calling setDefaults() is
		// strongly recommended.

	if (i2s_ctrl != 0x10)
		// paul had special case here which I generalized
		// if (((extMCLK > 0) && (i2s_ctrl == (0x0030 | (1<<7))) ))
	{
		LOG("SGTL5000 SOFT RESET DETECTED",0);

		// if so, do not initialize, instead
		// read the all important m_ana_ctrl, and
		// a few other registers that are cached,
		// then return.

		m_ana_ctrl = read(CHIP_ANA_CTRL);
		m_hp_muted = m_ana_ctrl & (1<<4) ? 1 : 0;
		m_lineout_muted = m_ana_ctrl & (1<<8) ? 1 : 0;

		for (uint16_t i=0; i<5; i++)
		{
			m_band_value[i] = read(DAP_AUDIO_EQ_BASS_BAND0+i);
			m_band_target[i] = m_band_value[i];
		}

		#if DUMP_CCS
			dumpCCValues("from start() soft reset");
		#endif

		LOG("start() returning from soft reset",0);
		return;
	}

	write(CHIP_ANA_POWER, 		0x4060);  	// VDDD is externally driven with 1.8V
	write(CHIP_LINREG_CTRL, 	0x006C);	// VDDA & VDDIO both over 3.1V
	write(CHIP_REF_CTRL, 		0x01F2);	// VAG=1.575, normal ramp, +12.5% bias current
	write(CHIP_LINE_OUT_CTRL, 	0x0F22);	// LO_VAGCNTRL=1.65V, OUT_CURRENT=0.54mA
	write(CHIP_SHORT_CTRL, 		0x4446); 	// allow up to 125mA
	write(CHIP_ANA_CTRL, 		0x0137);	// enable zero cross detectors
		//  0x100 = line out mute
		// !0x040 = dac->headphone (not bypass mode)
		//  0x020 = enable dac zero cross detector
		//  0x010 = mute headphone output
		//  0x004 = select line in input
		//  0x002 = enable adc zero crossing
		//  0x001 = mute analog volume

	#if USE_8MHZ_CLOCK

		// SGTL is I2S Master
		// Datasheet Pg. 14: Using the PLL - Asynchronous SYS_MCLK input
		// With 8Mhz, we write a zero to CHIP_CLK_TOP_CTRL

		write(CHIP_CLK_TOP_CTRL, 0);

		// From flowchart, then PLL_OUTPUT_FREQ=180.6336MHz
		// CHIP_PLL_CTRL->INT_DIVISOR=FLOOR(PLL_OUTPUT_FREQ/INPUT_FREQ)
		// CHIP_PLL_CTRL->FRAC_DIVISOR= ((PLL_OUTPUT_FREQ/INPUT_FREQ) -INT_DIVISOR) * 2048

		// int_div = floor(180.6336mhz / 8mhz) = floor(22.579125) = 22
		// frac_div = ((180.633mgz / 8mhz) - 22) * 2048
		//			= 0.579125 * 2048 = 1167.616 = 1167 as an integer

		//	// pllFreq = (4096.0l * AUDIO_SAMPLE_RATE_EXACT)
		//	uint32_t int_divisor = (pllFreq / extMCLK) & 0x1f;
		//	uint32_t frac_divisor = (uint32_t)((((float)pllFreq / extMCLK) - int_divisor) * 2048.0f) & 0x7ff;

		uint32_t int_divisor = 22;		// under 21 ao it fits
		uint32_t frac_divisor = 1167;	// under 2048 so it fits

		write(CHIP_PLL_CTRL, (int_divisor << 11) | frac_divisor);
		write(CHIP_ANA_POWER, 0x40FF | (1<<10) | (1<<8) ); // power up: lineout, hp, adc, dac, PLL_POWERUP, VCOAMP_POWERUP

	#else
		// SGTL is I2S Slave - prh this comment was incorrect for rPi
		write(CHIP_ANA_POWER, 0x40FF); // power up: lineout, hp, adc, dac
	#endif

    write(CHIP_DIG_POWER, 		0x0073);	// power up all digital stuff
    delay(400);
    write(CHIP_LINE_OUT_VOL, 	0x1D1D);	// default approx 1.3 volts peak-to-peak

	//	if (extMCLK > 0) {
	//		//SGTL is I2S Master
	//		write(CHIP_CLK_CTRL, 0x0004 | 0x03);  // 44.1 kHz, 256*Fs, use PLL
	//		write(CHIP_I2S_CTRL, 0x0030 | (1<<7)); // SCLK=64*Fs, 16bit, I2S format
	//	} else {
	//		//SGTL is I2S Slave
	//		write(CHIP_CLK_CTRL, 0x0004);  // 44.1 kHz, 256*Fs
	//		write(CHIP_I2S_CTRL, 0x0030); // SCLK=64*Fs, 16bit, I2S format
	//	}

    write(CHIP_CLK_CTRL,
		#if USE_8MHZ_CLOCK
          0x03 |                    // b0:1 = MCLK_FREQUENCY 0x0=256fs 0x3=USE_PLL
		#else
          0x00 |                    // b0:1 = MCLK_FREQUENCY 0x0=256fs !0x3= not using USE_PLL
		#endif
          0x04 |                    // b2:3 = 0x04 == 44.1khz
          0x00 );                   // rate mode
    
	// we were ALREADY setting the SGTL as the LRCLK/SCLK master
	
    write(CHIP_I2S_CTRL,            // default LR sense, is I2S
          0x00 |                    // b0 = 0 = don't invert LRCLCK
          0x00 |                    // b1 = 0 = data starts one BCLK after LR; 0x02 = no delay
          0x00 |                    // b2:3 = 0 = i2s mode, 0x4=rjust, 0x8=pcm, 
          0x30 |                    // b4:5 = 3 = i2s data length = 16 bits
          0x00 |                    // b6 = 0 = SCLK (BCLK) invert 
          0x80 |                    // b7 = 0x80 = Master LRCLK and SCLK (BCLK)
          0x00 );                   // b8 = 0 = SCLK frequencey=64fs (0x100==32fs)
    
	// prh - try increasing the i2s pad strengths to their maximum

	#if 1
		write(CHIP_PAD_STRENGTH,
				0x0001 |				// SCL drive strength = 1 = default
				0x0004 |				// SDA drive strength = 1 = default
				0x0010 |				// I2S dout drive strength = 1 = default
				0x00C0 |				// I2S SCLK = 3 = max
				0x0300 );				// I2S LRCLK = 3 = max
				// SCLK is sgtl5000's terminology for BCLK
	#endif


	write(CHIP_SSS_CTRL, 	0x0010);	// ADC->I2S, I2S->DAC
	write(CHIP_ADCDAC_CTRL, 0x0000);	// disable dac mute
	write(CHIP_DAC_VOL, 	0x3C3C);	// digital gain, 0dB
	write(CHIP_ANA_HP_CTRL, 0x7F7F);	// hp volume to lowest level (0 on my scale)

	// prh change - leave the lineout muted
	// in favor of allowing client to ramp the
	// lineout volume up ..

	write(CHIP_ANA_CTRL, 	0x0136);	// enable zero cross detectors
		// old !0x100 = unmute line out
		// new  0x100 = mute line out
		// !0x040 = dac->headphone
		//  0x020 = enable zero cross detector
		//  0x010 = mute headphone output
		//  0x004 = select line input
		//  0x002 = enable adc zero crossing
		// !0x001 = unmute analog (line in) volume

	#if DUMP_CCS
		dumpCCValues("at end of start()");
	#endif

    LOG("start() finished",0);
}

    


uint16_t AudioControlSGTL5000::read(uint16_t reg_num)
{
	uint16_t val;

	Wire.beginTransmission(m_i2c_addr);
	Wire.write(reg_num >> 8);
	Wire.write(reg_num);

	if (Wire.endTransmission() != 0)
	{
		LOG_ERROR("read(0x%04x,) failure1",reg_num);
		return 0;
	}

    u8 buf[2];
    CTimer::Get()->usDelay(100);
    if (Wire.read(m_i2c_addr,buf,2) < 2)
	{
		LOG_ERROR("read(0x%04x,) failure2",reg_num);
		return 0;
	}
    CTimer::Get()->usDelay(100);
    val = (buf[0] << 8) | buf[1];

    return val;
}


bool AudioControlSGTL5000::write(uint16_t reg_num, uint16_t val)
{
	Wire.beginTransmission(m_i2c_addr);
	Wire.write(reg_num >> 8);
	Wire.write(reg_num);
	Wire.write(val >> 8);
	Wire.write(val);
	if (Wire.endTransmission() != 0)
	{
		LOG_ERROR("write(0x%04x,%0x%04x) failure",reg_num,val);
		return false;
	}
	if (reg_num == CHIP_ANA_CTRL)
		m_ana_ctrl = val;
	return true;
}




bool AudioControlSGTL5000::modify(uint16_t reg_num, uint16_t val, uint16_t mask)
{
	uint16_t cur = read(reg_num);
	cur &= ~mask;
	cur |= val;
	if (!write(reg_num,cur)) return false;
	return true;
}


//----------------------------------------------
// virtual API
//----------------------------------------------

void AudioControlSGTL5000::volume(float n)
{
	setHeadphoneVolume(n * 128);
}
void AudioControlSGTL5000::inputLevel(float n)
{
	setLineInLevel(n * 15.5);	// 0..15
}


//----------------------------------------------------------------
// control API from left to right
//----------------------------------------------------------------

bool AudioControlSGTL5000::setDefaults()
	// The method is called by my code after enable().  Period.
	// Return the SGTL5000 to a somewhat known state.
	// See comments in enable() and sgtl5000midi.h
{
	DBG_API("setDefaults()",0);

	#define TEST_GUITAR_VALUES   1

	bool retval =

		// mute the sound

		setMuteHeadphone(1) &&
		setMuteLineOut(1) &&

		// set a bunch of stuff

		inputSelect(SGTL_INPUT_LINEIN) &&		// same as enable()
		setMicGain(1) &&						// my default
		setLineInLevel(7) &&					// my default
		setDacVolume(0) &&						// same as reset & enable()
		setDacVolumeRamp(DAC_VOLUME_RAMP_EXPONENTIAL) &&	// same as enable()
		setLineOutLevel(13) &&					// my default

		setHeadphoneSelect(HEADPHONE_NORMAL) &&	// same as reset & enable()
		setHeadphoneVolume(97) &&				// my default same as reset
		setAdcHighPassFilter(ADC_HIGH_PASS_ENABLE) &&	// same as reset
		setDapEnable(DAP_DISABLE) &&			// same as reset
		setSurroundEnable(SURROUND_DISABLED) &&	// same as reset
		setSurroundWidth(4) &&					// same as reset
		setEnableBassEnhance(0) &&				// same as reset
		setEnableBassEnhanceCutoff(0) &&		// same as reset
		setBassEnhanceCutoff(4) &&				// same as reset
		setBassEnhanceBoost(96) &&              // same as reset
		setBassEnhanceVolume(58) &&             // same as reset
		setEqSelect(EQ_FLAT) &&                 // same as reset
		setEqBand(0,15,true) &&                 // same as reset
		setEqBand(1,15,true) &&                 // same as reset
		setEqBand(2,15,true) &&                 // same as reset
		setEqBand(3,15,true) &&                 // same as reset
		setEqBand(4,15,true) &&                 // same as reset
		setAutoVolumeEnable(0) &&				// same as reset; not supported by midi

		#if TEST_GUITAR_LEVELS
			setLineInLevel(7) &&					// my default
			setLineOutLevel(18) &&					// my default
		#else
			setMuteHeadphone(0);
		#endif

		// unmute the sound
		setMuteLineOut(0);					// same as reset/enable()

	#if DUMP_CCS
		dumpCCValues("at end of setDefaults()");
	#endif

	return retval;
}


bool AudioControlSGTL5000::inputSelect(uint8_t val)
{
	DBG_API("inputSelect(%d)",val);
	if (val == SGTL_INPUT_MIC)			// 1 = AUDIO_INPUT_MIC
		return write(CHIP_ANA_CTRL, m_ana_ctrl & ~(1<<2)); // enable mic
	else // if (n == SGTL_INPUT_LINEIN)	// 0 = AUDIO_INPUT_LINEIN
		return write(CHIP_ANA_CTRL, m_ana_ctrl | (1<<2)); // enable linein
}
uint8_t AudioControlSGTL5000::getInputSelect()
{
	return (m_ana_ctrl & (1<<2)) ?
		SGTL_INPUT_LINEIN :
		SGTL_INPUT_MIC;
}


bool AudioControlSGTL5000::setMicGain(uint8_t val)
{
	DBG_API("setMicGain(%d)",val);
	return write(CHIP_MIC_CTRL, 0x0170 | val);
		// 0x070 = max 3.0V Mic Bias voltage
		// 0x100 = BIAS Reaistor 2.0 kOhm
}
uint8_t AudioControlSGTL5000::getMicGain()
{
	return read(CHIP_MIC_CTRL) & 0x3;
}


bool AudioControlSGTL5000::setLineInLevelLeft(uint8_t val)
	// Actual measured full-scale peak-to-peak sine wave input for max signal
	//  0: 3.12 Volts p-p
	//  1: 2.63 Volts p-p
	//  2: 2.22 Volts p-p
	//  3: 1.87 Volts p-p
	//  4: 1.58 Volts p-p
	//  5: 1.33 Volts p-p
	//  6: 1.11 Volts p-p
	//  7: 0.94 Volts p-p
	//  8: 0.79 Volts p-p
	//  9: 0.67 Volts p-p
	// 10: 0.56 Volts p-p
	// 11: 0.48 Volts p-p
	// 12: 0.40 Volts p-p
	// 13: 0.34 Volts p-p
	// 14: 0.29 Volts p-p
	// 15: 0.24 Volts p-p
{
	DBG_API("setLineInLevelLeft(%d)",val);
	if (val > 15) val = 15;
	return modify(CHIP_ANA_ADC_CTRL, val, 0xf);
}
bool AudioControlSGTL5000::setLineInLevelRight(uint8_t val)
{
	DBG_API("setLineInLevelRight(%d)",val);
	if (val > 15) val = 15;
	return modify(CHIP_ANA_ADC_CTRL, val << 4, 0xf << 4);
}
uint8_t AudioControlSGTL5000::getLineInLevelLeft()
{
	return read(CHIP_ANA_ADC_CTRL) & 0xf;
}
uint8_t AudioControlSGTL5000::getLineInLevelRight()
{
	return (read(CHIP_ANA_ADC_CTRL) & 0xf0) >> 4;
}



bool AudioControlSGTL5000::setDacVolumeLeft(uint8_t val)
	// chip: 0x3C = 0db, 0xF0=-90db, 0xFC=muted
	// we convert 0..126 to 0x3C..0xBB truncated range
	// and explicitly set 127 to 0xFC
{
	DBG_API("setDacVolumeLeft(%d)",val);
	//	uint16_t adcdac = read(CHIP_ADCDAC_CTRL);
	//	uint16_t dac_mute = adcdac & (1 << 2);
	//		// pull the left DAC_MUTE bit out of CHIP_ADCDAC_CTRL
	//	uint16_t should_mute = val == 127 ? 0 : 1 << 2;
	//  if (dac_mute != should_mute)
	//		modify(CHIP_ADCDAC_CTRL,should_mute,1<<2);
	//	if (should_mute)
	//		val = 0xFC;
	//	else

	val += 0x3C;
	return modify(CHIP_DAC_VOL,val,0xff);
}
bool AudioControlSGTL5000::setDacVolumeRight(uint8_t val)
{
	// DBG_API("setDacVolumeRight(%d)",val);
	// uint16_t adcdac = read(CHIP_ADCDAC_CTRL);
	// uint16_t dac_mute = adcdac & (2 << 2);
	// 	// pull the right DAC_MUTE bit out of CHIP_ADCDAC_CTRL
	// uint16_t should_mute = val == 127 ? 0 : 2 << 2;
	// if (dac_mute != should_mute)
	// 	modify(CHIP_ADCDAC_CTRL,should_mute,2<<2);
	// if (should_mute)
	// 	val = 0xFC;
	// else
	val += 0x3C;
	return modify(CHIP_DAC_VOL,val<<8,0xff00);
}
uint8_t AudioControlSGTL5000::getDacVolumeLeft()
{
	uint16_t val = read(CHIP_DAC_VOL) & 0xff;
	// if (val == 0xFC)
	//	return 127;
	return val - 0x3C;
}
uint8_t AudioControlSGTL5000::getDacVolumeRight()
{
	uint16_t val = (read(CHIP_DAC_VOL) >> 8) & 0xff;
	// if (val == 0xFC)
	//	return 127;
	return val - 0x3C;
}


bool AudioControlSGTL5000::setDacVolumeRamp(uint8_t val)
{
	DBG_API("setDacVolumeRamp(%d)",val);
	if (val == DAC_VOLUME_RAMP_DISABLE)
		return modify(CHIP_ADCDAC_CTRL, 0, 0x300);
	else if (val == DAC_VOLUME_RAMP_LINEAR)
		return modify(CHIP_ADCDAC_CTRL, 0x200, 0x300);
	// DAC_VOLUME_RAMP_EXPONENTIAL
	return modify(CHIP_ADCDAC_CTRL, 0x300, 0x300);
}
uint8_t AudioControlSGTL5000::getDacVolumeRamp()
{
	uint16_t val = read(CHIP_ADCDAC_CTRL) & 0x300;
	return
		val == 0x300 ? DAC_VOLUME_RAMP_EXPONENTIAL :
		val == 0x200 ? DAC_VOLUME_RAMP_LINEAR :
		DAC_VOLUME_RAMP_DISABLE;
}



bool AudioControlSGTL5000::setLineOutLevelLeft(uint8_t val)
	// CHIP_LINE_OUT_VOL
	//  Actual measured full-scale peak-to-peak sine wave output voltage:
	//  0-12: output has clipping		val  19..31
	//  13: 3.16 Volts p-p				18
	//  14: 2.98 Volts p-p				17
	//  15: 2.83 Volts p-p              16
	//  16: 2.67 Volts p-p              15
	//  17: 2.53 Volts p-p              14
	//  18: 2.39 Volts p-p              13
	//  19: 2.26 Volts p-p              12
	//  20: 2.14 Volts p-p              11
	//  21: 2.02 Volts p-p              10
	//  22: 1.91 Volts p-p              9
	//  23: 1.80 Volts p-p              8
	//  24: 1.71 Volts p-p              7
	//  25: 1.62 Volts p-p              6
	//  26: 1.53 Volts p-p              5
	//  27: 1.44 Volts p-p              4
	//  28: 1.37 Volts p-p              3
	//  29: 1.29 Volts p-p              2
	//  30: 1.22 Volts p-p              1
	//  31: 1.16 Volts p-p              0
	//
	// note: the register accepts 5 bits (0..63)
	// note: the reset default is 27 on my scale, preumably about 5v p-p
	//
	// apparently paul determined that values less than 13
	// and over 31 are not useful.
	//
	// enable() sets a starting value of 0x1D1D with a comment
	// 		of "approx 1.3 volts peak-to-peak". 0x1D == 29, so
	//      he must have been referring to his measured value of 1.29 volts,
	//	    which corresponds to my parameter of (2)
{
	DBG_API("setLineOutLevelLeft(%d)",val);
	// if (val && m_lineout_muted)
	//	setMuteLineOut(0);
	#if 1	// allow extended range
		if (val > 31) val = 31;
	#else
		if (val > 18) val = 18;
	#endif
	val = 31-val;
	return modify(CHIP_LINE_OUT_VOL,val,31);
}
bool AudioControlSGTL5000::setLineOutLevelRight(uint8_t val)
{
	DBG_API("setLineOutLevelRight(%d)",val);
	// if (val && m_lineout_muted)
	//	setMuteLineOut(0);
	#if 1	// allow extended range
		if (val > 31) val = 31;
	#else
		if (val > 18) val = 18;
	#endif

	val = 31-val;
	return modify(CHIP_LINE_OUT_VOL,val<<8,31<<8);
}
uint8_t AudioControlSGTL5000::getLineOutLevelLeft()
{
	return 31-(read(CHIP_LINE_OUT_VOL) & 31);
}
uint8_t AudioControlSGTL5000::getLineOutLevelRight()
{
	return 31 - ((read(CHIP_LINE_OUT_VOL) >> 8) & 31);
}


bool AudioControlSGTL5000::setHeadphoneSelect(uint8_t val)
{
	DBG_API("setHeadphoneSelect(%d)",val);
	if (val == HEADPHONE_LINEIN)		// bypass route LINE_IN to headphone
		return write(CHIP_ANA_CTRL, m_ana_ctrl | (1<<6));
	else // (val == HEADPHONE_NORMAL)	// route DAC to headphone
		return write(CHIP_ANA_CTRL, m_ana_ctrl & ~(1<<6));
}
uint8_t AudioControlSGTL5000::getHeadphoneSelect()
{
	return m_ana_ctrl & (1<<6) ?
		HEADPHONE_LINEIN :
		HEADPHONE_NORMAL;
}


bool AudioControlSGTL5000::setHeadphoneVolumeLeft(uint8_t val)	// 0..127
{
	DBG_API("setHeadphoneVolumeLeft(%d)",val);
	// if (val && m_hp_muted)
	//	setMuteHeadphone(0);
	if (val > 0x7f) val = 0x7f;
	val = 0x7f - val;
	return modify(CHIP_ANA_HP_CTRL, val, 0x7f);
}
bool AudioControlSGTL5000::setHeadphoneVolumeRight(uint8_t val)	// 0..127
{
	DBG_API("setHeadphoneVolumeRight(%d)",val);
	// if (val && m_hp_muted)
	//	setMuteHeadphone(0);
	if (val > 0x7f) val = 0x7f;
	val = 0x7f - val;
	return modify(CHIP_ANA_HP_CTRL, val << 8, 0x7f << 8);
}
uint8_t AudioControlSGTL5000::getHeadphoneVolumeLeft()
{
	return 0x7f-(read(CHIP_ANA_HP_CTRL) & 0x7f);
}
uint8_t AudioControlSGTL5000::getHeadphoneVolumeRight()
{
	return 0x7f-((read(CHIP_ANA_HP_CTRL) >> 8) & 0x7f);
}



bool AudioControlSGTL5000::setMuteHeadphone(uint8_t mute)
{
	DBG_API("setMuteHeadphone(%d)",mute);
	bool rslt;
	if (mute)
		rslt = write(CHIP_ANA_CTRL, m_ana_ctrl | (1<<4));
	else
		rslt = write(CHIP_ANA_CTRL, m_ana_ctrl & ~(1<<4));
	m_hp_muted = mute;
	return rslt;
}
uint8_t AudioControlSGTL5000::getMuteHeadphone()
{
	return m_ana_ctrl & (1<<4) ? 1 : 0;
}


bool AudioControlSGTL5000::setMuteLineOut(uint8_t mute)
{
	DBG_API("setMuteLineOut(%d)",mute);
	bool rslt;
	if (mute)
		rslt = write(CHIP_ANA_CTRL, m_ana_ctrl | (1<<8));
	else
		rslt = write(CHIP_ANA_CTRL, m_ana_ctrl & ~(1<<8));
	m_lineout_muted = mute;
	return rslt;
}
uint8_t AudioControlSGTL5000::getMuteLineOut()
{
	return m_ana_ctrl & (1<<8) ? 1 : 0;
}




bool AudioControlSGTL5000::setAdcHighPassFilter(uint8_t val)
{
	DBG_API("setAdcHighPassFilter(%d)",val);
	if (val == ADC_HIGH_PASS_DISABLE)
		return modify(CHIP_ADCDAC_CTRL, 1, 3);
	else if (val == ADC_HIGH_PASS_FREEZE)
		return modify(CHIP_ADCDAC_CTRL, 2, 3);
	else // ADC_HIGH_PASS_ENABLE
		return modify(CHIP_ADCDAC_CTRL, 0, 3);
}
uint8_t AudioControlSGTL5000::getAdcHighPassFilter()
{
	uint16_t val = read(CHIP_ADCDAC_CTRL)  & 0x3;
	if (val == 3)
		return ADC_HIGH_PASS_DISABLE;
	if (val == 2)
		return ADC_HIGH_PASS_FREEZE;
	return ADC_HIGH_PASS_ENABLE;
}



//---------------------------------------------------------------
// DAP_CONTROL
//---------------------------------------------------------------
// The documentation says outputs should be muted when changing
// the dapEnable setting.  This is implemented.
// It implies that the outputs should be muted when enabling
// or disabling the sub-blocks.  This is NOT implemented.

bool AudioControlSGTL5000::setDapEnable(uint8_t val)
	// Note that in all casees all other bits are zero, where
	//		bits 9:8 are DAP_MIX_SELECT(0) = DAP mixer <-- ADC
	//		and all higher bits are 0 to not swap L/R channels
	// The DAP mixer is disabled by DAP_CONTROL only being written as a 1
	//		or a zero.
{
	DBG_API("setDapEnable(%d)",val);

	bool save_hp_mute = m_hp_muted;
	bool save_lineout_mute = m_lineout_muted;
	if (!m_hp_muted)
		setMuteHeadphone(1);
	if (!m_lineout_muted)
		setMuteLineOut(1);
	bool result = false;

	if (val == DAP_ENABLE_POST)
		// audio processor used to post-process I2S in (teensy out) before DAC
		result = write(DAP_CONTROL, 1) && write(CHIP_SSS_CTRL, 0x0070);
			// 0xnnn0 = ADC    --> I2S_OUT
			// 0xnn4n = I2S_IN --> DAP
			// 0xnn3n = DAP    --> DAC
	else if (val == DAP_ENABLE_PRE)
		// audio processor used to pre-process analog input (ADC) before I2S_OUT (teensy in)
		result = write(DAP_CONTROL, 1) && write(CHIP_SSS_CTRL, 0x0013);
			// 0xn0nn = ADC    --> DAP
			// 0xnnn3 = DAP    --> I2S_OUT
			// 0xnn1n = ISS_IN --> DAC
	else	// DAP_DISABLE
		result = write(CHIP_SSS_CTRL, 0x0010) && write(DAP_CONTROL, 0);
			// 0xnnn0 = ADC    --> I2S_OUT
			// 0xnn1n = ISS_IN --> DAC

	if (save_hp_mute != m_hp_muted)
		setMuteHeadphone(save_hp_mute);
	if (save_lineout_mute != m_lineout_muted)
		setMuteLineOut(save_lineout_mute);

	return result;
}
uint8_t AudioControlSGTL5000::getDapEnable()
{
	uint16_t dap_control = read(DAP_CONTROL);
	uint16_t sss_control = read(CHIP_SSS_CTRL);
	if (!dap_control)
		return DAP_DISABLE;
	if ((sss_control & 0x0013) == 0x0013)
		return DAP_ENABLE_PRE;
	return DAP_ENABLE_POST;
}




//--------------------------
// DAP AVC
//--------------------------
// Not currently supported via MIDI interface

bool AudioControlSGTL5000::setAutoVolumeEnable(uint8_t enable)
{
	DBG_API("setAutoVolumeEnable(%d)",enable);

	if (enable)
		return modify(DAP_AVC_CTRL, 1, 1);
	else
		return modify(DAP_AVC_CTRL, 0, 1);
}


bool AudioControlSGTL5000::setAutoVolumeControl(uint8_t maxGain, uint8_t lbiResponse, uint8_t hardLimit, float threshold, float attack, float decay)
	// Valid values for dap_avc parameters
	//
	// maxGain; Maximum gain that can be applied
	// 0 - 0 dB
	// 1 - 6.0 dB
	// 2 - 12 dB
    //
	// lbiResponse; Integrator Response
	// 0 - 0 mS
	// 1 - 25 mS
	// 2 - 50 mS
	// 3 - 100 mS
    //
	// hardLimit
	// 0 - Hard limit disabled. AVC Compressor/Expander enabled.
	// 1 - Hard limit enabled. The signal is limited to the programmed threshold (signal saturates at the threshold)
    //
	// threshold
	// floating point in range 0 to -96 dB
    //
	// attack
	// floating point figure is dB/s rate at which gain is increased
    //
	// decay
	// floating point figure is dB/s rate at which gain is reduced

{
	// if(m_semi_automated&&(!read(DAP_CONTROL)&1)) audioProcessorEnable();

	DBG_API("setAutoVolumeControl(%d,%d,%d,%0.3f,%0.3f,0.3f)",
			maxGain,lbiResponse,hardLimit,threshold,attack,decay);

	if (maxGain > 2) maxGain = 2;
	lbiResponse &= 3;
	hardLimit &= 1;

	uint16_t thresh=(pow(10,threshold/20)*0.636)*pow(2,15);
	// uint8_t thresh=(pow(10,threshold/20)*0.636)*pow(2,15);
		// A couple of observations about pauls original code.
		// The DAP_AVC_THRESHOLD accepts a full 16 bit value.
		// Pauls math appears to be correct to convert DB into a full range 16bit register value,
		// However, Paul original code put it in a uint8_t that he sent to write().
		// This means that for inputs of less than -48db, the results would be unpredictable.
		// So, to start with I am changing it to an uint16_t.
		// Doc:
		//
		//		Threshold is programmable. Use the following formula to calculate hex value:
		//		Hex Value = ((10^(THRESHOLD_dB/20))*0.636)*2^15
		//		Threshold can be set in the range of 0 dB to -96 dB
		//		Example Values:
		//		0x1473 = Set Threshold to -12 dB
		//		0x0A40 = Set Threshold to -18 dB
		//
		// After much conversation with an AI, I was able to suss out the following:
		//
		//		larger register values result in a lower dB threshold
		//		0xffff = -0.043dB
		//		0x0000 = -96db
		//	    there is more precision available at lower negative dB's than
		//		at higher ones.
		//
		// How this all plays out is not clear to me.
		// Most guitar compressors use a "ratio"

	uint8_t att=(1-pow(10,-(attack/(20*44100))))*pow(2,19);
	uint8_t dec=(1-pow(10,-(decay/(20*44100))))*pow(2,23);
		// I am afraid to change these to uint16_ts.
		// The register values are 11 bits.
		// Gonna leave it for now.

	return
		write(DAP_AVC_THRESHOLD,thresh) &&
		write(DAP_AVC_ATTACK,att) &&
		write(DAP_AVC_DECAY,dec) &&
		modify(DAP_AVC_CTRL,
			maxGain << 12 |
			lbiResponse << 8 |
			hardLimit << 5,
			3<<12|3<<8|1<<5);
}



//--------------------------
// DAP Surround Sound
//--------------------------

bool AudioControlSGTL5000::setSurroundEnable(uint8_t val)
	// 0 == disabled
	// 1 == mondo
	// 2 == stereo
{
	DBG_API("setSurroundEnable(%d)",val);
	if (val) val += 1;
	return modify(DAP_SGTL_SURROUND, val & 0x3, 0x3);
}

bool AudioControlSGTL5000::setSurroundWidth(uint8_t width)
{
	DBG_API("setSurroundWidth(%d)",width);
	return modify(DAP_SGTL_SURROUND,(width&7)<<4,0x7<<4);
}

uint8_t AudioControlSGTL5000::getSurroundEnable()
{
	uint16_t val = read(DAP_SGTL_SURROUND) & 0x3;
	if (val == 3)
		return SURROUND_STEREO;
	if (val == 2)
		return SURROUND_MONO;
	return SURROUND_DISABLED;
}
uint8_t AudioControlSGTL5000::getSurroundWidth()
{
	return (read(DAP_SGTL_SURROUND)>>4) & 0x7;
}



//-----------------------
// DAP Bass Enhance
//-----------------------


bool AudioControlSGTL5000::setEnableBassEnhance(uint8_t enable)
{
	DBG_API("setEnableBassEnhance(%d)",enable);
	return modify(DAP_BASS_ENHANCE, enable & 1, 1);
}
bool AudioControlSGTL5000::setEnableBassEnhanceCutoff(uint8_t enable)
{
	DBG_API("setEnableBassEnhanceCutoff(%d)",enable);
	return modify(DAP_BASS_ENHANCE, (enable & 1)<<8, 1<<8);
}
bool AudioControlSGTL5000::setBassEnhanceCutoff(uint8_t freq)
{
	DBG_API("setBassEnhanceCutoff(%d)",freq);
	return modify(DAP_BASS_ENHANCE, (freq & 7)<<4, 7<<4);
}
bool AudioControlSGTL5000::setBassEnhanceBoost(uint8_t val)
{
	DBG_API("setBassEnhanceBoost(%d)",val);
	if (val > 0x7f) val = 0x7f;
	return modify(DAP_BASS_ENHANCE_CTRL, 0x7F-val, 0x7F);
}
bool AudioControlSGTL5000::setBassEnhanceVolume(uint8_t val)
{
	DBG_API("setBassEnhanceVolume(%d)",val);
	if (val > 0x3f) val = 0x3f;
	return modify(DAP_BASS_ENHANCE_CTRL, (0x3f-val)<<8, 0x3f<<8);
}

uint8_t AudioControlSGTL5000::getEnableBassEnhance()
{
	return read(DAP_BASS_ENHANCE) & 1;
}
uint8_t AudioControlSGTL5000::getEnableBassEnhanceCutoff()
{
	return (read(DAP_BASS_ENHANCE) >> 8) & 1;
}
uint8_t AudioControlSGTL5000::getBassEnhanceCutoff()
{
	return (read(DAP_BASS_ENHANCE) >> 4) & 0x7;
}
uint8_t AudioControlSGTL5000::getBassEnhanceBoost()
{
	return 0x7f - (read(DAP_BASS_ENHANCE_CTRL) & 0x7f);
}
uint8_t AudioControlSGTL5000::getBassEnhanceVolume()
{
	return 0x3f-((read(DAP_BASS_ENHANCE_CTRL) >> 8) & 0x3f);
}






//------------------
// DAP_AUDIO_EQ
//------------------

bool AudioControlSGTL5000::setEqSelect(uint8_t val)
	// 0 = NONE,
	// 1 = PEQ (paramaterized eq 7 IIR Biquad filters)
	// 2 = TONE (bass and treble tone control)
	// 3 = GEQ (5 band graphic EQ)
{
	DBG_API("setEqSelect(%d)",val);
	return modify(DAP_AUDIO_EQ,val & 0x3 , 0x3);
}
uint8_t AudioControlSGTL5000::getEqSelect()
{
	return read(DAP_AUDIO_EQ) & 0x3;
}


// single entry point for TONE(2) and GEQ(3)
// 		works with loop() automation
// Note that client must call setEnableDap() and
//		setEqSelect(2 or 3) before calling this method.
// assumes that m_band_value[5] is correct.

#define AUTOMATION_BUSY  (1 << 6)	// 6th bit
#define AUTIMATION_MASK	 (0x1f)		// 1st 5 bits

bool AudioControlSGTL5000::setEqBand(uint8_t band_num, uint8_t val, bool force /* = 0 */)	// 0..95 (0x5F)
	// for ToneControl use 0 and 4
	// for GE use 0..4
	// Sets EQ band gain from -11.75db to +12db in 0.25db steps.
	// reset default is 47 (0x2f) = 0 db
{
	DBG_API("setEqBand(%d,%d)",band_num,val);

	if (val > 0x5f) val = 0x5f;
	m_band_target[band_num] = val;
	if (force)
	{
		m_band_value[band_num] = val;
		write(DAP_AUDIO_EQ_BASS_BAND0+(band_num*2),val);
	}
	else
	{
		m_in_automation |= (1 << band_num);
		if (!(m_in_automation & AUTOMATION_BUSY))
		{
			m_in_automation |= AUTOMATION_BUSY;
			handleEqAutomation(band_num);
			m_in_automation &= ~AUTOMATION_BUSY;
		}
	}

	return 1;
}
uint8_t AudioControlSGTL5000::getEqBand(uint8_t band_num)
{
	return read(DAP_AUDIO_EQ_BASS_BAND0+(band_num*2)) & 0x5f;
}



void AudioControlSGTL5000::handleEqAutomation(uint8_t band_num)
	// returns true if automation should continue
	// no good way to return a write failure
{
	int desired = m_band_target[band_num];
	int cur = m_band_value[band_num];

	if (desired - cur > 2)
	{
		DBG_AUTO("handleEqAutomation band(%d) desired(%d) cur(%d) set(%d)",band_num,desired,cur,cur+2);
		cur = cur + 2;
	}
	else if (desired - cur < -2)
	{
		DBG_AUTO("handleEqAutomation band(%d) desired(%d) cur(%d) set(%d)",band_num,desired,cur,cur-2);
		cur = cur - 2;
	}
	else	// done with this band
	{
		DBG_AUTO("handleEqAutomation band(%d) desired(%d) cur(%d) done",band_num,desired,cur);
		cur = desired;
		m_in_automation &= ~(1<<band_num);
	}

	m_band_value[band_num] = cur;
	write(DAP_AUDIO_EQ_BASS_BAND0+(band_num*2),cur);
}



void AudioControlSGTL5000::loop()
{
	if ( (m_in_automation & AUTIMATION_MASK) &&
		!(m_in_automation & AUTOMATION_BUSY))
	{
		DBG_AUTO("loop() m_in_automation(%04x)",m_in_automation);

		m_in_automation |= AUTOMATION_BUSY;
		for (uint8_t band_num=0; band_num<5; band_num++)
		{
			if (m_in_automation & (1<<band_num))
				handleEqAutomation(band_num);
		}
		m_in_automation &= ~AUTOMATION_BUSY;
	}
}



// EQ(1) = Complicated PEQ (Parameteriszed EQ) methods
// Not currently supported via MIDI interface

uint16_t AudioControlSGTL5000::eqFilterCount(uint8_t n) // valid to n&7, 0 thru 7 filters enabled.
{
	return modify(DAP_PEQ,(n&7),7);
}

void AudioControlSGTL5000::eqFilter(uint8_t filterNum, int *filterParameters)
	// SGTL5000 PEQ Coefficient loader
{
	// TODO: add the part that selects 7 PEQ filters.
	// if (m_semi_automated) automate(1,1,filterNum+1);

	modify(DAP_FILTER_COEF_ACCESS,(uint16_t)filterNum,15);
	write(DAP_COEF_WR_B0_MSB,(*filterParameters>>4)&65535);
	write(DAP_COEF_WR_B0_LSB,(*filterParameters++)&15);
	write(DAP_COEF_WR_B1_MSB,(*filterParameters>>4)&65535);
	write(DAP_COEF_WR_B1_LSB,(*filterParameters++)&15);
	write(DAP_COEF_WR_B2_MSB,(*filterParameters>>4)&65535);
	write(DAP_COEF_WR_B2_LSB,(*filterParameters++)&15);
	write(DAP_COEF_WR_A1_MSB,(*filterParameters>>4)&65535);
	write(DAP_COEF_WR_A1_LSB,(*filterParameters++)&15);
	write(DAP_COEF_WR_A2_MSB,(*filterParameters>>4)&65535);
	write(DAP_COEF_WR_A2_LSB,(*filterParameters++)&15);
	write(DAP_FILTER_COEF_ACCESS,(uint16_t)0x100|filterNum);
}



void AudioControlSGTL5000::calcBiquad(uint8_t filtertype, float fC, float dB_Gain, float Q, uint32_t quantization_unit, uint32_t fS, int *coef)
	// PEQ parameter helper method
	// if(SGTL5000_PEQ) quantization_unit=524288; if(AudioFilterBiquad) quantization_unit=2147483648;
{
	// I used resources like http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
	// to make this routine, I tested most of the filter types and they worked. Such filters have limits and
	// before calling this routine with varying values the end user should check that those values are limited
	// to valid results.

	float A;
	float b0,b1,b2,a0,a1,a2;

	if (filtertype < FILTER_PARAEQ)
		A = pow(10,dB_Gain/20);
	else
		A=pow(10,dB_Gain/40);
	float W0 = 2*3.14159265358979323846*fC/fS;
	float cosw = cosf(W0);
	float sinw = sinf(W0);
	//float alpha = sinw*sinh((log(2)/2)*BW*W0/sinw);
	//float beta = sqrt(2*A);
	float alpha = sinw / (2 * Q);
	float beta = sqrtf(A)/Q;

	switch(filtertype)
	{
		case FILTER_LOPASS:
			b0 = (1.0F - cosw) * 0.5F; // =(1-COS($H$2))/2
			b1 = 1.0F - cosw;
			b2 = (1.0F - cosw) * 0.5F;
			a0 = 1.0F + alpha;
			a1 = 2.0F * cosw;
			a2 = alpha - 1.0F;
			break;
		case FILTER_HIPASS:
			b0 = (1.0F + cosw) * 0.5F;
			b1 = -(cosw + 1.0F);
			b2 = (1.0F + cosw) * 0.5F;
			a0 = 1.0F + alpha;
			a1 = 2.0F * cosw;
			a2 = alpha - 1.0F;
			break;
		case FILTER_BANDPASS:
			b0 = alpha;
			b1 = 0.0F;
			b2 = -alpha;
			a0 = 1.0F + alpha;
			a1 = 2.0F * cosw;
			a2 = alpha - 1.0F;
			break;
		case FILTER_NOTCH:
			b0=1;
			b1=-2*cosw;
			b2=1;
			a0=1+alpha;
			a1=2*cosw;
			a2=-(1-alpha);
			break;
		case FILTER_PARAEQ:
			b0 = 1 + (alpha*A);
			b1 =-2 * cosw;
			b2 = 1 - (alpha*A);
			a0 = 1 + (alpha/A);
			a1 = 2 * cosw;
			a2 =-(1-(alpha/A));
			break;
		case FILTER_LOSHELF:
			b0 = A * ((A+1.0F) - ((A-1.0F)*cosw) + (beta*sinw));
			b1 = 2.0F * A * ((A-1.0F) - ((A+1.0F)*cosw));
			b2 = A * ((A+1.0F) - ((A-1.0F)*cosw) - (beta*sinw));
			a0 = (A+1.0F) + ((A-1.0F)*cosw) + (beta*sinw);
			a1 = 2.0F * ((A-1.0F) + ((A+1.0F)*cosw));
			a2 = -((A+1.0F) + ((A-1.0F)*cosw) - (beta*sinw));
			break;
		case FILTER_HISHELF:
			b0 = A * ((A+1.0F) + ((A-1.0F)*cosw) + (beta*sinw));
			b1 = -2.0F * A * ((A-1.0F) + ((A+1.0F)*cosw));
			b2 = A * ((A+1.0F) + ((A-1.0F)*cosw) - (beta*sinw));
			a0 = (A+1.0F) - ((A-1.0F)*cosw) + (beta*sinw);
			a1 = -2.0F * ((A-1.0F) - ((A+1.0F)*cosw));
			a2 = -((A+1.0F) - ((A-1.0F)*cosw) - (beta*sinw));
			break;
		default:
			b0 = 0.5;
			b1 = 0.0;
			b2 = 0.0;
			a0 = 1.0;
			a1 = 0.0;
			a2 = 0.0;
	}

	a0 = (a0*2)/(float)quantization_unit; // once here instead of five times there...
	b0 /= a0;
	*coef++ = (int)(b0+0.499);
	b1 /= a0;
	*coef++ = (int)(b1+0.499);
	b2 /= a0;
	*coef++ = (int)(b2+0.499);
	a1 /= a0;
	*coef++ = (int)(a1+0.499);
	a2 /=a0;
	*coef++ = (int)(a2+0.499);
}


//-------------------------------------------------
// MIDI API
//-------------------------------------------------

#if 0

	void AudioControlSGTL5000::dumpCCValues(const char *where)
	{
		display(0,"sgtl CC values %s",where);
		proc_entry();
		for (uint8_t cc=SGTL_CC_BASE; cc<=SGTL_CC_MAX; cc++)
		{
			if (!sgtl5000_writeOnlyCC(cc))
			{
				int val = getCC(cc);
				display(0,"SGTL_CC(%-2d) = %-4d  %-20s max=%d",cc,val,sgtl5000_getCCName(cc),sgtl5000_getCCMax(cc));
			}
			else if (0)
				display(0,"",0);
		}
		proc_leave();
	}


	bool AudioControlSGTL5000::dispatchCC(uint8_t cc, uint8_t val)
	{
		display(dbg_dispatch,"sgtl500 CC(%d) %s <= %d",cc,sgtl5000_getCCName(cc),val);

		switch (cc)
		{
			case SGTL_CC_DUMP					: dumpCCValues("from dump_sgtl command"); return 1;
			case SGTL_CC_SET_DEFAULTS			: return setDefaults();
			case SGTL_CC_INPUT_SELECT			: return inputSelect(val);
			case SGTL_CC_MIC_GAIN_				: return setMicGain(val);
			case SGTL_CC_LINEIN_LEVEL			: return setLineInLevel(val);
			case SGTL_CC_LINEIN_LEVEL_LEFT		: return setLineInLevelLeft(val);
			case SGTL_CC_LINEIN_LEVEL_RIGHT		: return setLineInLevelRight(val);
			case SGTL_CC_DAC_VOLUME				: return setDacVolume(val);
			case SGTL_CC_DAC_VOLUME_LEFT		: return setDacVolumeLeft(val);
			case SGTL_CC_DAC_VOLUME_RIGHT		: return setDacVolumeRight(val);
			case SGTL_CC_DAC_VOLUME_RAMP		: return setDacVolumeRamp(val);
			case SGTL_CC_LINEOUT_LEVEL			: return setLineOutLevel(val);
			case SGTL_CC_LINEOUT_LEVEL_LEFT		: return setLineOutLevelLeft(val);
			case SGTL_CC_LINEOUT_LEVEL_RIGHT	: return setLineOutLevelRight(val);
			case SGTL_CC_HP_SELECT				: return setHeadphoneSelect(val);
			case SGTL_CC_HP_VOLUME				: return setHeadphoneVolume(val);
			case SGTL_CC_HP_VOLUME_LEFT			: return setHeadphoneVolumeLeft(val);
			case SGTL_CC_HP_VOLUME_RIGHT		: return setHeadphoneVolumeRight(val);
			case SGTL_CC_MUTE_HP				: return setMuteHeadphone(val);
			case SGTL_CC_MUTE_LINEOUT			: return setMuteLineOut(val);
			case SGTL_CC_ADC_HIGH_PASS			: return setAdcHighPassFilter(val);
			case SGTL_CC_DAP_ENABLE				: return setDapEnable(val);
			case SGTL_CC_SURROUND_ENABLE		: return setSurroundEnable(val);
			case SGTL_CC_SURROUND_WIDTH			: return setSurroundWidth(val);
			case SGTL_CC_BASS_ENHANCE_ENABLE	: return setEnableBassEnhance(val);
			case SGTL_CC_BASS_CUTOFF_ENABLE		: return setEnableBassEnhanceCutoff(val);
			case SGTL_CC_BASS_CUTOFF_FREQ		: return setBassEnhanceCutoff(val);
			case SGTL_CC_BASS_BOOST				: return setBassEnhanceBoost(val);
			case SGTL_CC_BASS_VOLUME			: return setBassEnhanceVolume(val);
			case SGTL_CC_EQ_SELECT				: return setEqSelect(val);
			case SGTL_CC_EQ_BAND0				: return setEqBand(0,val);
			case SGTL_CC_EQ_BAND1				: return setEqBand(1,val);
			case SGTL_CC_EQ_BAND2				: return setEqBand(2,val);
			case SGTL_CC_EQ_BAND3				: return setEqBand(3,val);
			case SGTL_CC_EQ_BAND4				: return setEqBand(4,val);
		}

		my_error("unknown dispatchCC(%d,%d)",cc,val);
		return false;
	}


	int AudioControlSGTL5000::getCC(uint8_t cc)
	{
		int val = -1;

		switch (cc)
		{
			case SGTL_CC_DUMP					: val = 255; 							break;
			case SGTL_CC_SET_DEFAULTS			: val = 255; 							break;
			case SGTL_CC_INPUT_SELECT			: val = getInput();						break;
			case SGTL_CC_MIC_GAIN_				: val = getMicGain();	                break;
			case SGTL_CC_LINEIN_LEVEL			: val = 255; 							break;
			case SGTL_CC_LINEIN_LEVEL_LEFT		: val = getLineInLevelLeft();	        break;
			case SGTL_CC_LINEIN_LEVEL_RIGHT		: val = getLineInLevelRight();	        break;
			case SGTL_CC_DAC_VOLUME				: val = 255; 							break;
			case SGTL_CC_DAC_VOLUME_LEFT		: val = getDacVolumeLeft();	            break;
			case SGTL_CC_DAC_VOLUME_RIGHT		: val = getDacVolumeRight();	        break;
			case SGTL_CC_DAC_VOLUME_RAMP		: val = getDacVolumeRamp();	            break;
			case SGTL_CC_LINEOUT_LEVEL			: val = 255; 							break;
			case SGTL_CC_LINEOUT_LEVEL_LEFT		: val = getLineOutLevelLeft();	        break;
			case SGTL_CC_LINEOUT_LEVEL_RIGHT	: val = getLineOutLevelRight();	        break;
			case SGTL_CC_HP_SELECT				: val = getHeadphoneSelect();	        break;
			case SGTL_CC_HP_VOLUME				: val = 255; 							break;
			case SGTL_CC_HP_VOLUME_LEFT			: val = getHeadphoneVolumeLeft();	    break;
			case SGTL_CC_HP_VOLUME_RIGHT		: val = getHeadphoneVolumeRight();	    break;
			case SGTL_CC_MUTE_HP				: val = getMuteHeadphone();	            break;
			case SGTL_CC_MUTE_LINEOUT			: val = getMuteLineOut();	            break;
			case SGTL_CC_ADC_HIGH_PASS			: val = getAdcHighPassFilter();	        break;
			case SGTL_CC_DAP_ENABLE				: val = getDapEnable();	                break;
			case SGTL_CC_SURROUND_ENABLE		: val = getSurroundEnable();	        break;
			case SGTL_CC_SURROUND_WIDTH			: val = getSurroundWidth();	            break;
			case SGTL_CC_BASS_ENHANCE_ENABLE	: val = getEnableBassEnhance();	        break;
			case SGTL_CC_BASS_CUTOFF_ENABLE		: val = getEnableBassEnhanceCutoff();	break;
			case SGTL_CC_BASS_CUTOFF_FREQ		: val = getBassEnhanceCutoff();	        break;
			case SGTL_CC_BASS_BOOST				: val = getBassEnhanceBoost();	        break;
			case SGTL_CC_BASS_VOLUME			: val = getBassEnhanceVolume();	        break;
			case SGTL_CC_EQ_SELECT				: val = getEqSelect();	                break;
			case SGTL_CC_EQ_BAND0				: val = getEqBand(0);	                break;
			case SGTL_CC_EQ_BAND1				: val = getEqBand(1);	                break;
			case SGTL_CC_EQ_BAND2				: val = getEqBand(2);	                break;
			case SGTL_CC_EQ_BAND3				: val = getEqBand(3);	                break;
			case SGTL_CC_EQ_BAND4				: val = getEqBand(4);	                break;
		}

		return val;
	}

#endif // 0


// end of control_sgtl5000.cpp

