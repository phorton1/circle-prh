
#include <audio\Audio.h>

// You may only define zero or one of the following
// the default is wm8731

#define USE_CS42448   0
#define USE_STGL5000  1

#if USE_CS42448

    // Octo is always the master
    
    AudioInputTDM input;
    AudioOutputTDM output;
    AudioControlCS42448 control;

#elif USE_STGL5000  // only in slave mode

    // #if I2S_MASTER
    // the rpi cannot be a master to an sgtl5000.
    // the sgtl5000 requires 3 clocks and the rpi can only generate 2
    //
    // AudioInputI2S input;
    // AudioOutputI2S output;
    // AudioControlSGTL5000 control;
    // #else
    
    AudioInputI2Sslave input;
    AudioOutputI2Sslave output;
    AudioControlSGTL5000master control;

    // #endif
    
#else   // wm8731 in master or slave mode

    #define I2S_MASTER    0

    // Is the BCM_PCM the master or slave device?
    // Of the devices so far, only the wm8731 can be a master.
    // Paul usually thinks the teensy is the master device,
    // and adds suffixes to the class names when it's not, but
    // the rpi generally behaves better as the i2s slave than
    // as the master.
    
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



