
#include <audio\Audio.h>

#define USE_CS42448  1

#if USE_CS42448

    AudioInputTDM input;
    AudioOutputTDM output;
    AudioControlCS42448 control;

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


AudioConnection  c0(input, 0, output, 0);
AudioConnection  c1(input, 0, output, 1);




void setup()
{
    printf("02-StereoPassThru::setup()\n");

    control.enable();
    
    AudioMemory(80);
    
    #if USE_CS42448
        control.setSampleRate(44100);
    #else
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
    #endif
    
    control.volume(1.0);

    
    printf("02-StereoPassThru::setup() finished\n");
}



void loop()
{
}



