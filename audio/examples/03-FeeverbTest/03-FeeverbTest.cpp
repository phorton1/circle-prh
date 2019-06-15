
#include <audio\Audio.h>

// There is a funny twang every so often in FreeVerb,
// maybe having to do with integer overflows.

#define USE_MIXER       0
#define USE_FREEVERB    0


#define USE_CS42448  1

#if USE_CS42448

    AudioInputTDM input;
    AudioOutputTDM output;
    AudioControlCS42448 control;
    AudioControlCS42448 *pControl;

#else

    #define I2S_MASTER   1

    // There seems to be some noise on the right channel
    // with the rpi in master mode with the wm8731 
    
    #if I2S_MASTER
        AudioInputI2S input;
        AudioOutputI2S output;
        AudioControlWM8731 control;
    #else
        AudioInputI2Sslave input;
        AudioOutputI2Sslave output;
        AudioControlWM8731master control;
    #endif

#endif


#if USE_FREEVERB
    AudioEffectFreeverbStereo   reverb;
#else
    AudioEffectReverb   reverb1;
    AudioEffectReverb   reverb2;
#endif

#if USE_MIXER
    AudioMixer4 mixer1;
    AudioMixer4 mixer2;
#endif


#if USE_FREEVERB
    AudioConnection  c1(input, 0, reverb, 0);
    AudioConnection  c2(input, 1, reverb, 1);
#else
    AudioConnection  c1(input, 0, reverb1, 0);
    AudioConnection  c2(input, 1, reverb2, 0);
#endif


#if USE_MIXER
    AudioConnection  c3(input,   0, mixer1, 0);
    AudioConnection  c4(input,   1, mixer2, 0);

    #if USE_FREEVERB
        AudioConnection  c5(reverb,  0, mixer1, 1);
        AudioConnection  c6(reverb,  1, mixer2, 1);
    #else
        AudioConnection  c5(reverb1, 0, mixer1, 1);
        AudioConnection  c6(reverb2, 0, mixer2, 1);
    #endif

    AudioConnection  c7(mixer1,  0, output, 0);
    AudioConnection  c8(mixer2,  0, output, 1);
#else
    #if USE_FREEVERB
        AudioConnection  c7(reverb,  0, output, 0);
        AudioConnection  c8(reverb,  1, output, 1);
    #else
        AudioConnection  c7(reverb1, 0, output, 0);
        AudioConnection  c8(reverb2, 0, output, 1);
    #endif
#endif


void setup()
{
    printf("03-FreeverbTest::setup(blah)\n");
    
    #if USE_FREEVERB
        reverb.roomsize(0.6);
        reverb.damping(0.5);
    #else
        reverb1.reverbTime(0.8);
        reverb2.reverbTime(0.8);
    #endif

    #if USE_CS42448
        pControl = &control;
        control.reset();    // the reset must be done before enable
    #endif
    
    control.enable();
        
    AudioMemory(100);

    #if !USE_CS42448
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
        control.volume(1.0);
    #endif
        
    #if USE_MIXER
        mixer1.gain(0, 0.6);
        mixer1.gain(1, 0.35);
        mixer2.gain(0, 0.6);
        mixer2.gain(1, 0.35);
    #endif
    
    printf("03-FreeverbTest::setup() finished\n");
}


void loop()
{
}




