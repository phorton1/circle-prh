
#include <audio\Audio.h>

// note that you must use 'master' control devices
// if you use 'slave' i2s devices.

#define I2S_MASTER  0
    // I am now getting NOISE on the rpi-as-master input right channel
    
#define USE_SYNTH   0
#define USE_REVERB  1


#if USE_SYNTH
    AudioSynthWaveformSine modulate;
    AudioSynthWaveformSineModulated input;
    AudioConnection c0(modulate, 0, input, 0);
    #define INPUT0 input
    #define INPUT1 input
    #define INPUT_CH0 0
    #define INPUT_CH1 0
#else
    #if I2S_MASTER
        AudioInputI2S input;
    #else
        AudioInputI2Sslave input;
    #endif
    #define INPUT0      input
    #define INPUT1      input
    #define INPUT_CH0   0
    #define INPUT_CH1   1
#endif

#if USE_REVERB
    // can only have ONE reverb and ONLY on the LEFT channel ?!?!
    AudioEffectReverb   reverb1;
    AudioEffectReverb   reverb2;
    AudioConnection     c1(INPUT0, INPUT_CH0, reverb1, 0);
    AudioConnection     c2(INPUT1, INPUT_CH1, reverb2, 0);
    #define FINAL_INPUT0 reverb1
    #define FINAL_INPUT1 reverb2
    #define FINAL_CH0    0
    #define FINAL_CH1    0
#else
    #define FINAL_INPUT0 INPUT0
    #define FINAL_INPUT1 INPUT1
    #define FINAL_CH0    INPUT_CH0
    #define FINAL_CH1    INPUT_CH1
#endif

AudioMixer4 mixer1;
AudioMixer4 mixer2;

#if I2S_MASTER
    AudioOutputI2S output;
    AudioControlWM8731 control;
#else
    AudioOutputI2Sslave output;
    AudioControlWM8731master control;
#endif



AudioConnection  c3(FINAL_INPUT0, FINAL_CH0, mixer1, 0);
AudioConnection  c4(INPUT0,       INPUT_CH0, mixer1, 1);
AudioConnection  c5(FINAL_INPUT0, FINAL_CH0, mixer2, 0);
AudioConnection  c6(INPUT0,       INPUT_CH0, mixer2, 1);

AudioConnection  c7(mixer1, 0, output, 0);
AudioConnection  c8(mixer2, 0, output, 1);



void setup()
{
    printf("hardware_test::setup()\n");
    
    // the wm8731 master clock is a little fast, A#

    #if USE_SYNTH
        modulate.frequency(0);
        input.frequency(440.0);
    #endif
    
    #if USE_REVERB
        reverb1.reverbTime(0.6);
    #endif
    

    // my version of AudioMemory() also calls the
    // register() and begin() methods on each object.

    AudioMemory(20);
    
    control.enable();
    control.inputSelect(AUDIO_INPUT_LINEIN);
    control.inputLevel(1.0);
    #if USE_SYNTH
        control.volume(0.0);
    #else
        control.volume(1.0);
    #endif
    
    mixer1.gain(0, 0.3);
    mixer1.gain(1, 0.5);
    mixer2.gain(0, 0.3);
    mixer2.gain(1, 0.5);
    
    printf("hardware_test::setup() finished\n");
}



void loop()
{
    #if USE_SYNTH
        static int counter = 0;
        if (counter++ < 6)
        {
            control.volume(counter & 1 ? 0.6 : 0.00);
            delay(500);
        }
        else
        {
            counter = 0;
            delay(2000);
        }
    #endif
}



