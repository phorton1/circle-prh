
#include <audio\Audio.h>

#define USE_CS42448  0

#if USE_CS42448

    AudioControlCS42448 control;

#else

    // There seems to be some noise on the right channel
    // with the rpi in master mode with the wm8731 
    
    #define I2S_MASTER   1
    #if I2S_MASTER
        AudioInputI2S input;
        AudioOutputI2S output;
        AudioControlWM8731 control;
    #else
        AudioInputI2Sslave input;
        AudioOutputI2Sslave output;
        AudioControlWM8731master control;
    #endif

    AudioConnection  c0(input, 0, output, 0);
    AudioConnection  c1(input, 1, output, 1);

#endif






void setup()
{
    printf("02-StereoPassThru::setup()\n");

    AudioMemory(20);
    
    control.enable();
    control.volume(1.0);
    
    #if USE_CS42448
        control.volume(1,1.0);
        control.volume(2,1.0);
        control.inputLevel(1,1.0);
        control.inputLevel(2,1.0);
    #else
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
    #endif
    
    printf("02-StereoPassThru::setup() finished\n");
}



void loop()
{
}



