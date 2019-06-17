
#include <audio\Audio.h>

#define USE_CS42448  1

#if USE_CS42448

    AudioInputTDM input;
    AudioOutputTDM output;
    AudioControlCS42448 control;

#else

    #define I2S_MASTER   0
        // is the BCM_PCM the master or slave device
        // Paul usually thinks the teensy is the master device,
        // and adds suffixes to the class names when it's not.
        // The rpi behaves better as the i2s clave than as the master.
    
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
    
    #if !USE_CS42448
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
    #endif
    
    control.volume(1.0);

    
    printf("02-StereoPassThru::setup() finished\n");
}



void loop()
{
}



