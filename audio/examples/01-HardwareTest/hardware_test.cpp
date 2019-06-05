
#include <audio\Audio.h>

// note that you must use 'master' control devices
// if you use 'slave' i2s devices.

#define USE_SYNTH   1

#if USE_SYNTH
    AudioSynthWaveformSine           modulate;
    AudioSynthWaveformSineModulated  input;
    AudioConnection                  patchCord(modulate, 0, input, 0);
#endif

#if 1
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
#if USE_SYNTH
    AudioConnection         patchCord2(input, 0, output, 1);
#else
    AudioConnection         patchCord2(input, 1, output, 1);
#endif



void setup()
{
    printf("hardware_test::setup()\n");
    
    // the wm8731 master clock is a little fast, A#

    #if USE_SYNTH
        modulate.frequency(0);
        input.frequency(440.0);
    #endif

    // my version of AudioMemory() also calls the
    // register() and begin() methods on each object.

    AudioMemory(10);
    
    control.enable();
    control.inputSelect(AUDIO_INPUT_LINEIN);
    control.inputLevel(1.0);
    #if USE_SYNTH
        control.volume(0.0);
    #else
        control.volume(1,0);
    #endif
    
    printf("hardware_test::setup() finished\n");
}



int counter = 0;

void loop()
{
    #if USE_SYNTH
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



