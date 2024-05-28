//-------------------------------------------------------------------------
// control_sgtl5000.h
//-------------------------------------------------------------------------
// Audio Library for Teensy 3.X
// Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
// please see LICENSE.TXT.
//
// This API has been reworked for clarity and control by MIDI.
// It is envisioned that client code will have some way of persistently
// storing the SGTL5000 "configuration" and will initialize all gains,
// senstivities, filters, and DAP blocks for the given application.


#pragma once

#include "AudioDevice.h"
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>


#define SGTL5000_I2C_ADDR		0x0A
#define SGTL5000_I2C_ADDR_ALT	0x2A


//-------------------------------------------------------
// sgtl5000 CC numbers and parameters
//-------------------------------------------------------
// enumerated method paramters

#define	SGTL_INPUT_LINEIN	0	// same as AUDIO_INPUT_LINEIN
#define SGTL_INPUT_MIC		1	// same as AUDIO_INPUT_MIC
	// params to setInput()

#define HEADPHONE_NORMAL	0	// normal mode
#define HEADPHONE_LINEIN	1	// bypass mode
	// param to setHeadphoneSelect()

#define DAC_VOLUME_RAMP_EXPONENTIAL 0
#define DAC_VOLUME_RAMP_LINEAR  	1
#define DAC_VOLUME_RAMP_DISABLE  	2
	// param to setDacVolumeRamp()

#define ADC_HIGH_PASS_ENABLE		0
#define ADC_HIGH_PASS_FREEZE		1
#define ADC_HIGH_PASS_DISABLE		2
	// param to setAdcHighPassFilter()

#define DAP_DISABLE					0
#define DAP_ENABLE_PRE				1
#define DAP_ENABLE_POST				2
	// param to setDapEnable()

#define SURROUND_DISABLED			0
#define SURROUND_MONO				1
#define SURROUND_STEREO				2
	// param to setSurroundEnable()

#define EQ_FLAT 					0
#define EQ_PARAM_EQ 				1
#define EQ_TONE_CONTROLS			2
#define EQ_GRAPHIC_EQ				3
	// param to setEqSelect()

#define EQ_BAND0					0 	// 115 Hz = TONE_CONTROLS(BASS)
#define EQ_BAND1					1 	// 330 Hz
#define EQ_BAND2					2 	// 990 Hz
#define EQ_BAND3					3 	// 3000 Hz
#define EQ_BAND4					4	// 9900 Hz = TONE_CONTROLS(TREBLE)
	// param to setEqBand

#define FILTER_LOPASS 				0
#define FILTER_HIPASS 				1
#define FILTER_BANDPASS 			2
#define FILTER_NOTCH 				3
#define FILTER_PARAEQ 				4
#define FILTER_LOSHELF 				5
#define FILTER_HISHELF 				6
	// params to PEQ method


//----------------------------------------
// CC Numbers
//----------------------------------------

#define SGTL_CC_BASE					30

// Values on my scales							//	MAX				RESET	ENABLE	setDefaults					notes
//																					x = reset to sgtl->enable defaults
#define SGTL_CC_DUMP					30		//  Write Only
#define SGTL_CC_SET_DEFAULTS			31		//	Write Only
#define SGTL_CC_INPUT_SELECT			32      //	1				1  		0  		x							enable() switches to line input
#define SGTL_CC_MIC_GAIN_				33      //	3               0       		1 = 20db
#define SGTL_CC_LINEIN_LEVEL			34		//	Write Only
#define SGTL_CC_LINEIN_LEVEL_LEFT		35      //	15              0       		7 = middle of range
#define SGTL_CC_LINEIN_LEVEL_RIGHT		36      //	15              0       		7
#define SGTL_CC_DAC_VOLUME				37		//	Write Only
#define SGTL_CC_DAC_VOLUME_LEFT			38      //	127             0       0		x							126=-63db; 127=muted; enable() explicitly re-sets the default value
#define SGTL_CC_DAC_VOLUME_RIGHT		39      //	127             0       0		x							126=-63db; 127=muted; enable() explicitly re-sets the default value
#define SGTL_CC_DAC_VOLUME_RAMP			40      //	2               1       0  		x							enable() enables exponential, not linear, ramping
#define SGTL_CC_LINEOUT_LEVEL			41		//	Write Only
#define SGTL_CC_LINEOUT_LEVEL_LEFT		42      //	31              27      2  		13 = middle of range, but	enable() explicitly sets LINE_OUT levels
#define SGTL_CC_LINEOUT_LEVEL_RIGHT		43      //	31              27      2		13 = upper of paul's 0..18	enable() explicitly sets LINE_OUT levels
#define SGTL_CC_HP_SELECT				44      //	1               0       0		x							enable() explicitly write CHIP_ANA_CTRL, including this bit
#define SGTL_CC_HP_VOLUME				45		//	Write Only
#define SGTL_CC_HP_VOLUME_LEFT			46      //	127             97      0		97							enable() currently turns the headphones all the way down
#define SGTL_CC_HP_VOLUME_RIGHT			47      //	127             97      0		97							enable() currently turns the headphones all the way down
#define SGTL_CC_MUTE_HP					48      //	1               0       1  		0							enable() explicitly write CHIP_ANA_CTRL, including this bit
#define SGTL_CC_MUTE_LINEOUT			49      //	1               0       0		0							enable() explicitly write CHIP_ANA_CTRL, including this bit
#define SGTL_CC_ADC_HIGH_PASS			50      //	2               0				x
#define SGTL_CC_DAP_ENABLE				51      //	2               0				x
#define SGTL_CC_SURROUND_ENABLE			52      //	2               0			    x
#define SGTL_CC_SURROUND_WIDTH			53      //	7               4               x
#define SGTL_CC_BASS_ENHANCE_ENABLE		54      //	1               0               x
#define SGTL_CC_BASS_CUTOFF_ENABLE		55      //	1               0               x
#define SGTL_CC_BASS_CUTOFF_FREQ		56      //	6               4               x
#define SGTL_CC_BASS_BOOST				57      //	127             96              x
#define SGTL_CC_BASS_VOLUME				58      //	63              58              x
#define SGTL_CC_EQ_SELECT				59      //	3               0               x
#define SGTL_CC_EQ_BAND0				60      //	95              15              x
#define SGTL_CC_EQ_BAND1				61      //	95              15              x
#define SGTL_CC_EQ_BAND2				62      //	95              15              x
#define SGTL_CC_EQ_BAND3				63      //	95              15              x
#define SGTL_CC_EQ_BAND4				64      //	95              15              x

// #define SGTL_CC_AVC					65		// unimpelmented/complicated 	 	DISABLED in setDefaults but AVC specific registers are currently not reset
// #define SGTL_CC_PEQ					66		// unimplemented/complicated 		DISABLED in setDefaults but PEQ specific registers are currently not reset

#define SGTL_CC_MAX						64


//-------------------------------------------------------
// class definition
//-------------------------------------------------------

class AudioControlSGTL5000 : public AudioCodec
	// Client may call setDefaults() for a reliable setup of reasonable values.
	// Otherwise, client may call the the methods associated with the [bracketed] blocks.
	//
	//				 (bypass)
	//					+----------------------------------------------------------------------------------+
	//                  |                                                                                  |--> [HP_VOLUME] --> HP_OUT
	//   LINE_IN -------+--------+                                                                         |
	//                           |---> [LINEIN_LEVEL] --> ADC --> [SWITCH] --> [DAC_VOLUME] --> DAC ---+---+
	//   MIC ---> [MIC_GAIN] ----+      (ANALOG_GAIN)               |  ^                               |
	//												 			    v  |							   +------> [LINEOUT_VOL] --> LINE_OUT
	//
{
public:
	
	AudioControlSGTL5000();
	
	void setAltAddress()  { m_i2c_addr = SGTL5000_I2C_ADDR_ALT; }
	
	const char *getName() override { return "sgtl5000"; }

	void start() override;
	void volume(float n) override;
		// sets headphone volume at this time
	void inputLevel(float n) override;
		// sets lineInLevel 0..15


	void loop();
		// after enable(), loop() is necessary if using the
		// TONE(2) or GEQ(3) eq setters, as the documentation
		// says that to avoid clicks these registers must not
		// be arbitrarily changed, but rather, only ramped up
		// in 0.5db (increments of 2 in the uint8_t values).


	// these methods return false if they fail to write to the SGTL5000
	// there are separate accessors for "getting" register values.

	bool setDefaults();
		// See sgtl50000.cpp and sgtl5000midi.h for more information.
		// Reserts all registers to stable useful inital values.


	//----------------------------------------------------
	// control API from left to right
	//----------------------------------------------------
	// These are represented by incrementing Midi CC numbers,
	// and all take uint8_t parameters in the range 0..127.

	bool inputSelect(uint8_t val);	// 0..1
	uint8_t getInputSelect();
		// Uses denormalized or teeensy audio system constants.
		//		0 = SGTL_INPUT_LINEIN	= AUDIO_INPUT_LINEIN
		//		1 = SGTL_INPUT_MIC      = AUDIO_INPUT_MIC
		// Does not set or change any gains or other characteristics

	bool setMicGain(uint8_t val);	// 0..3
	uint8_t getMicGain();
		// 0 = 0db
		// 1 = 20db
		// 2 = 30db
		// 3 = 40db

	bool setLineInLevelLeft(uint8_t val);	// 0..15
	bool setLineInLevelRight(uint8_t val);
	bool setLineInLevel(uint8_t val) { return
			setLineInLevelLeft(val) &&
			setLineInLevelRight(val); }
	uint8_t getLineInLevelLeft();
	uint8_t getLineInLevelRight();
		// Sets the ANALOG_GAIN independent of the MIC_GAIN.
		// in 1.5db steps from 0 to 22.5db. See implementation
		// for table of measured p-p voltages. Note the register
		// supports a 6db attenuation bit to make it go from
		// -6.0db to 16.5db if needed.

	bool setDacVolumeLeft(uint8_t val);	// 0..127 INVERTED!
	bool setDacVolumeRight(uint8_t val);
	bool setDacVolume(uint8_t val) { return
			setDacVolumeLeft(val) &&
			setDacVolumeRight(val); }
	uint8_t getDacVolumeLeft();
	uint8_t getDacVolumeRight();
		// ATTENUATE (turn down) the digital signal before the DAC
		// in 0.5db steps, from 0 to -63db, or mute it entirely.
		//
		// 		0 = 0 db (no attentuation)
		// 		1 = -0.5 db
		// 		2 = -1.0 db
		//		...
		//		125 = -62.5db
		//		126 = -63db
		//		127 = -63.5
		//
		// Note that to accomodate the 0..127 midi CC value range, we do not
		// access the full SGTL5000 range of -90db available for this register,
		// and also note that this method does not change DAC Mute state.
	bool setDacVolumeRamp(uint8_t val);	// 0..2
	uint8_t getDacVolumeRamp();
		// control ramping of dac
		// 0 = normal (exponential), 1=linear, 2=disabled

	bool setLineOutLevelLeft(uint8_t val);		// 0..31 (see note)
	bool setLineOutLevelRight(uint8_t val);
	bool setLineOutLevel(uint8_t val) { return
			setLineOutLevelLeft(val) &&
			setLineOutLevelRight(val); }
	uint8_t getLineOutLevelLeft();
	uint8_t getLineOutLevelRight();
		// Sets the LINE_OUT volume in 0.5db steps
		//		from some arbitrary starting point.
		// This register is complicated.
		// At some point Paul measured the device and determined
		// 		that values above 18 (on my scale) would lead to
		//		clipping.
		// Paul's original uint8_t API had wacky allowed values of 13..31,
		//  	and was "backwards" where higher numbers
		//      result in less p-p output voltage with 13=3.3V p-p
		//		and 31=1.66V p-p
		// I use his scale, but, I believe, with the "correct" sense
		//		where bigger numbers result in bigger output p-p
		//		voltages and I allow it to be overdriven to 31
		// On my scale, 0=1.66v p-p and 18=3.16v p-p, paul's max.
		// NOTE: does not change mute state

	bool setHeadphoneSelect(uint8_t val);	// 0..1
	uint8_t getHeadphoneSelect();
		// The headphone amplifier can be connected to the DAC (HEADPHONE_NORMAL)
		// or "bypass mode" (HEADPHONE_LINEIN) that routes the LINE_IN directly
		// to the headphone amp. When in bypass mode, setLineInLevel() has no effect.
		// but setHeadphoneVolume() && setMuteHeadphone() still work.

	bool setHeadphoneVolumeLeft(uint8_t val);	// 0..127;
	bool setHeadphoneVolumeRight(uint8_t val);
	bool setHeadphoneVolume(uint8_t val) { return
			setHeadphoneVolumeLeft(val) &&
			setHeadphoneVolumeRight(val); }
	uint8_t getHeadphoneVolumeLeft();
	uint8_t getHeadphoneVolumeRight();
		// Adjusts the HEADPHONE amplifier from -51.5 db to +12db
		// in 0.5db steps. headPhoneVolume(97) = 0db
		// NOTE: does not change mute state

	bool setMuteHeadphone(uint8_t mute);
	bool setMuteLineOut(uint8_t mute);
	uint8_t getMuteHeadphone();
	uint8_t getMuteLineOut();
		// These single bit modifiers do what they say.

	bool setAdcHighPassFilter(uint8_t val);	// 0..2
	uint8_t getAdcHighPassFilter();
		// oddball API.
		// 		0 = ADC_HIGH_PASS_ENABLE
		// 		1 = ADC_HIGH_PASS_FREEZE
		// 		2 = ADC_HIGH_PASS_DISABLE
		// Disabling the ADC High Pass filter may give better bass response,
		// but allow DC noise in. Freezing it is supported, but weird, as it
		// is an ongoing input-sensitive automatic filter.


	//------------------------------------------
	// DAP Blocks
	//------------------------------------------
	// MIX unused, AVC and PEQ not supported by MIDI
	//
	// switch -> MIX --> AVC --> SURROUND --> BASS_ENHANCE --> TONE_CONTROL --> switch
	//           +6db    +12b                 +6db             +12db
	//
	// The MIX block is generally disabled in this implementation.

	bool setDapEnable(uint8_t val);	// 0..2
	uint8_t getDapEnable();
		// Disable the DAP, or enable it pre or post I2S
		// 		0 = DAP_DISABLE
		// 		1 = DAP_ENABLE_PRE before I2S_OUT
		// 		2 = DAP_ENABLE_POST after I2S_IN
		// "It is good practice to mute the outputs before enabling or
		//  disabling the Audio Processor to avoid clicks or thumps."


	// AVC not currently supported by MIDI
	bool setAutoVolumeEnable(uint8_t enable);
		// limiter/compressor stage
	bool setAutoVolumeControl(uint8_t maxGain, uint8_t lbiResponse, uint8_t hardLimit, float threshold, float attack, float decay);
		// Configures the auto volume control, which is implemented as a
		// compressor/expander or hard limiter. maxGain is the maximum gain that
		// can be applied for expanding, and can take one of three values:
		// 0 (0dB), 1 (6.0dB) and 2 (12dB). Values greater than 2 are treated as 2.
		// response controls the integration time for the compressor and can take four values:
		// 0 (0ms), 1 (25ms), 2 (50ms) or 3 (100ms).
		// Larger values average the volume over a longer time, allowing short-term peaks through.
		//
		// If hardLimit is 0, a 'soft knee' compressor is used to progressively compress louder
		// values which are near to or above the threashold (the louder they are,
		// the greater the compression). If it is 1, a hard compressor is used
		// (all values above the threashold are the same loudness). The threashold is
		// specified as a float in the range 0dBFS to -96dBFS, where -18dBFS is a typical value.
		// attack is a float controlling the rate of decrease in gain when the signal is over threashold,
		// in dB/s. decay controls how fast gain is restored once the level drops below threashold, again in dB/s.
		// It is typically set to a longer value than attack.

	// SURROUND
	bool setSurroundEnable(uint8_t enable);	// 0..2
		// 0 = disabled
		// 1 = mono
		// 2 = stereo
	bool setSurroundWidth(uint8_t width);	// 0..7
		// 0 to 7 (widest)
	uint8_t getSurroundEnable();
	uint8_t getSurroundWidth();

	// BASS_ENHANCE
	bool setEnableBassEnhance(uint8_t enable);				// 0..1
	bool setEnableBassEnhanceCutoff(uint8_t enable);		// 0..1
	bool setBassEnhanceCutoff(uint8_t freq);				// 0..6
		// 0 =  80Hz
		// 1 = 100Hz
		// 2 = 125Hz
		// 3 = 150Hz
		// 4 = 175Hz
		// 5 = 200Hz
		// 6 = 225Hz
	bool setBassEnhanceBoost(uint8_t val);					// 0..0x7f
		// sets amount of harmonics boost.
		// default = 0x60
	bool setBassEnhanceVolume(uint8_t val);					// 0..0x3f
		// set level up to +6dB
		// default = 58 on my scale
	uint8_t getEnableBassEnhance();
	uint8_t getEnableBassEnhanceCutoff();
	uint8_t getBassEnhanceCutoff();
	uint8_t getBassEnhanceBoost();
	uint8_t getBassEnhanceVolume();


	// TONE_CONTROL
	// PEQ(1) not supported by MIDI
	// For the other two, TONE(2) and GEQ(3), in order to avoid pops,
	// the changes are automated to occur in no more than 0.5db steps.
	// Hence, this SGTL5000 has a loop() method that must be called to
	// handle this automation.

	bool setEqSelect(uint8_t n);	// 0..3
	uint8_t getEqSelect();
		// Selects the type of frequency control,
		// FLAT_FREQUENCY (0) Equalizers and tone controls disabled, flat frequency response.
		// PARAMETRIC_EQUALIZER (1) Enables the 7-band parametric equalizer
			void eqFilter(uint8_t filterNum, int *filterParameters);
				// Configurs the parametric equalizer. The number of filters (1 to 7) is
				// specified along with a pointer to an array of filter coefficients.
				// The parametric equalizer is implemented using 7 cascaded, second order
				// bi-quad filters whose frequencies, gain, and Q may be freely configured,
				// but each filter can only be specified as a set of filter coefficients.
			uint16_t eqFilterCount(uint8_t n);
				// Enables zero or more of the already configured parametric filters.
			void calcBiquad(uint8_t filtertype, float fC, float dB_Gain, float Q, uint32_t quantization_unit, uint32_t fS, int *coef);
				// Helper method to build filter parameters
		// TONE_CONTROLS (2) Enables bass and treble tone controls
		// GRAPHIC_EQUALIZER (3) Enables the five-band graphic equalizer
			bool setEqBand(uint8_t band_num, uint8_t val, bool force = false);	// 0..95 (0x5F)
				// for TONE(2) use 0 and 4
				// for GE(3) use 0..4
				// Sets EQ band gain from -11.75db to +12db in 0.25db steps.
				// reset default is 47 (0x2f) = 0 db
			uint8_t getEqBand(uint8_t band_num);
				// returns the actual current value
				// not having to do with automation


	//-------------------------------
	// MIDI implementation
	//-------------------------------

	// bool dispatchCC(uint8_t cc, uint8_t val);
	// int getCC(uint8_t cc);
	// 	// uses #SGTL_CC_XXXX constants and 0..127 uint8_t parameters
	// 	// all setters are defined
	// 	// getCC returns -1 for unknown CC numbers,
	// 	//		255 for write only or mondadic commands
    //
	// // debugging support
    //
	// void dumpCCValues(const char *where);		// debugging dump of everything


protected:

	uint8_t m_i2c_addr;
	
	CGPIOPin   m_MPIN;
	CGPIOClock m_MCLK;

	bool m_hp_muted;
	bool m_lineout_muted;
	uint16_t m_ana_ctrl;

	// automation variables
	// note that the user must call loop()

	uint8_t m_band_value[5];
	uint8_t m_band_target[5];
	volatile uint8_t m_in_automation;
		// bitwise bands that need automation
		// with 1<<6 as a busy flag

	bool write(uint16_t reg_num, uint16_t val);
		// returns 0 on failure, 1 on success
	uint16_t read(uint16_t reg_num);
		// note that API cannot differentiate between
		// a read failure, and read of a register containing zero
	bool modify(uint16_t reg_num, uint16_t val, uint16_t mask);
		// returns 1 if the write() succeeds, or zero if it fails.
		// can fail to function properly and still return 1 due to
		// API limitation of read()

	// utilities

	void handleEqAutomation(uint8_t band_num);

	
};





