// 2019-06-03 start porting to Circle

#ifndef Audio_h_
#define Audio_h_

#define AudioNoInterrupts() 
#define AudioInterrupts()   

#include <Arduino.h>
#include "Wire.h"
#include "AudioDevice.h"
#include "AudioStream.h"
#include "AudioConnection.h"
#include "AudioSystem.h"
#include "analyze_peak.h"
#include "analyze_rms.h"
#include "control_wm8731.h"
#include "control_cs42448.h"
#include "control_sgtl5000.h"
#include "effect_reverb.h"
#include "effect_freeverb.h"
#include "input_i2s.h"
#include "input_tdm.h"
#include "input_teensy_quad.h"
#include "mixer.h"
#include "output_i2s.h"
#include "output_tdm.h"
#include "output_teensy_quad.h"
#include "recorder.h"
#include "synth_sine.h"


#if 0   // unported teensy classes
    
    #include "analyze_fft256.h"
    #include "analyze_fft1024.h"
    #include "analyze_print.h"
    #include "analyze_tonedetect.h"
    #include "analyze_notefreq.h"
    #include "analyze_peak.h"
    #include "analyze_rms.h"
    #include "control_ak4558.h"
    #include "control_cs4272.h"
    #include "control_tlv320aic3206.h"
    #include "effect_bitcrusher.h"
    #include "effect_chorus.h"
    #include "effect_fade.h"
    #include "effect_flange.h"
    #include "effect_envelope.h"
    #include "effect_multiply.h"
    #include "effect_delay.h"
    #include "effect_delay_ext.h"
    #include "effect_midside.h"
    #include "effect_waveshaper.h"
    #include "effect_granular.h"
    #include "effect_combine.h"
    #include "filter_biquad.h"
    #include "filter_fir.h"
    #include "filter_variable.h"
    #include "input_adc.h"
    #include "input_adcs.h"
    #include "input_i2s_quad.h"
    #include "input_pdm.h"
    #include "output_dac.h"
    #include "output_dacs.h"
    #include "output_i2s_quad.h"
    #include "output_pwm.h"
    #include "output_spdif.h"
    #include "output_pt8211.h"
    #include "output_adat.h"
    #include "play_memory.h"
    #include "play_queue.h"
    #include "play_sd_raw.h"
    #include "play_sd_wav.h"
    #include "play_serialflash_raw.h"
    #include "record_queue.h"
    #include "synth_tonesweep.h"
    #include "synth_waveform.h"
    #include "synth_dc.h"
    #include "synth_whitenoise.h"
    #include "synth_pinknoise.h"
    #include "synth_karplusstrong.h"
    #include "synth_simple_drum.h"
    #include "synth_pwm.h"
    #include "synth_wavetable.h"

#endif  // unported teensy classes

#endif  // !Audio_h_
