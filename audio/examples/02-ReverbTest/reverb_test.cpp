#include <Arduino.h>
    // Arduino.h is not needed but it is fun to list it
    // here as if it were a real arduino program :-)
    
#include <audio\Audio.h>


// note that you must use 'master' control devices
// if you use 'slave' i2s devices.

#define I2S_MASTER  1
#define USE_MIXER   1
    
#if I2S_MASTER
    AudioInputI2S input;
#else
    AudioInputI2Sslave input;
#endif

AudioEffectReverb   reverb1;
AudioEffectReverb   reverb2;

#if USE_MIXER
    AudioMixer4 mixer1;
    AudioMixer4 mixer2;
#endif

#if I2S_MASTER
    AudioOutputI2S output;
    AudioControlWM8731 control;
#else
    AudioOutputI2Sslave output;
    AudioControlWM8731master control;
#endif


AudioConnection  c1(input, 0, reverb1, 0);
AudioConnection  c2(input, 1, reverb2, 0);

#if USE_MIXER
    AudioConnection  c3(input,   0, mixer1, 0);
    AudioConnection  c4(reverb1, 0, mixer1, 1);
    AudioConnection  c5(input,   1, mixer2, 0);
    AudioConnection  c6(reverb2, 0, mixer2, 1);

    AudioConnection  c7(mixer1,  0, output, 0);
    AudioConnection  c8(mixer2,  0, output, 1);
#else
    AudioConnection  c7(reverb1, 0, output, 0);
    AudioConnection  c8(reverb2, 0, output, 1);
#endif


void setup()
{
    printf("reverb_test::setup()\n");
    
    reverb1.reverbTime(0.6);
    reverb2.reverbTime(0.6);

    AudioMemory(20);

    control.enable();
    control.inputSelect(AUDIO_INPUT_LINEIN);
    control.inputLevel(1.0);
    control.volume(1.0);

    #if USE_MIXER
        mixer1.gain(0, 0.6);
        mixer1.gain(1, 0.3);
        mixer2.gain(0, 0.6);
        mixer2.gain(1, 0.3);
    #endif
    
    printf("reverb_test::setup() finished\n");
}


void loop()
{
}


