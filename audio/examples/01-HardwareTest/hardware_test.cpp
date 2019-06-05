
#include <audio\Audio.h>

// note that you must use 'master' control devices
// if you use 'slave' i2s devices.

#define USE_SYNTH   0

#if USE_SYNTH
    AudioSynthWaveformSine  synth;
    AudioSynthWaveformSine  input;
#endif

#if 0
    // rpi master, codec slave
    AudioControlWM8731          control;
    #if !USE_SYNTH
        AudioInputI2S           input;
    #endif
    AudioOutputI2S              output;
#else
    // rpi slave, codec master
    AudioControlWM8731master    control;
    #if !USE_SYNTH
        AudioInputI2Sslave      input;
    #endif
    AudioOutputI2Sslave         output;
#endif


AudioConnection             patchCord1(input, 0, output, 0);
AudioConnection             patchCord2(input, 1, output, 1);



void setup()
{
    printf("hardware_test::setup()\n");


    #if USE_SYNTH
        synth.frequency(440.0);
        input.frequency(440.0);
    #endif

    // my version of AudioMemory() also calls the
    // register() and begin() methods on each object.
    // There is a question of whether this should be done
    // BEFORE or AFTER the wm8731 is setup.

    
    control.enable();
    control.inputSelect(AUDIO_INPUT_LINEIN);
    control.inputLevel(1.0);
    control.volume(1.0);

    AudioMemory(10);
    
    printf("hardware_test::setup() finished\n");
}



void loop()
{
}



