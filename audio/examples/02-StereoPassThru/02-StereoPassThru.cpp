
#include <audio\Audio.h>

// You may define zero or one of the following
// the default is wm8731

#define USE_CS42448             1
#define USE_STGL5000            0
#define USE_TEENSY_QUAD_SLAVE   0
#define WITH_PROBE              1


#if USE_CS42448

    // Octo is always the master
    AudioInputTDM input;
    AudioOutputTDM output;
    AudioControlCS42448 control;

#elif USE_TEENSY_QUAD_SLAVE

    AudioInputTeensyQuad   input;
    AudioOutputTeensyQuad  output;

#elif USE_STGL5000  // only in slave mode

    // the rpi cannot be a master to an sgtl5000.
    // the sgtl5000 requires 3 clocks and the rpi can only generate 2
    AudioInputI2Sslave input;
    AudioOutputI2Sslave output;
    AudioControlSGTL5000master control;
    
#else   // wm8731 in master or slave mode

    #define I2S_MASTER    0
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

#if WITH_PROBE
    AudioProbe probe(0);
    AudioConnection c2(input, 0, probe, 0);
    AudioConnection c3(input, 1, probe, 1);
    #if USE_CS42448
        AudioConnection c4(input, 2, probe, 2);
        AudioConnection c5(input, 3, probe, 3);
    #endif    
#endif


void setup()
{
    printf("02-StereoPassThru::setup()\n");
    
    #if !USE_TEENSY_QUAD_SLAVE
        control.enable();
    #endif
    
    AudioMemory(80);
    
    #if !USE_CS42448 && !USE_TEENSY_QUAD_SLAVE
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
    #endif
    
    #if !USE_TEENSY_QUAD_SLAVE
        // ramp up the master volume over 1 second
        for (u16 i=0; i<=50; i++)
        {
            control.volume(((float)i) / 50.0);
            delay(20);
        }
    #endif
    
    printf("02-StereoPassThru::setup() finished\n");
}



void loop()
{
}



