// 02-StereoPassThru.cpp example program
//
// This, the second audio example, merely passes the buffers
// from the input device to the output device to verify that
// both input and outputs work.  For the Octo, the six inputs
// are mapped to the first 6 outputs.

#include <audio\Audio.h>

// You must define one of the following.

#define USE_WM8731              0
#define USE_CS42448             0
#define USE_STGL5000            1
#define USE_TEENSY_QUAD_SLAVE   0

// prh 2024-05-27 - this program has not been tested in ages.
//
// Initial attempt to turn on SGTL5000 revealed that I removed
// the AudioControlSGTL5000master, AudioInputI2Sslave and
// AudioOutputI2Sslave audio components a long time ago.
// So to get it to compile, I use the "regular" sgtl5000 and
// i2s devices, but I'm almost sure that even if it compiles
// it won't work.
//
// I almost certainly need to look at the control_sgtl5000
// implementation and compare the way it uses the bcm_pcm
// to later cs42488/wm8731 usages and do some heavy lifting
// to fix it.  Then I am concerned that the rPi MCLK will
// not be good and the sound will be bad.


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
    AudioInputI2S input;
    AudioOutputI2S output;
        // AudioInputI2Sslave input;
        // AudioOutputI2Sslave output;

    AudioControlSGTL5000 control;
        // prh 2024-05-27 verified that this IS the previous
        // AudioControlSGTL5000master control;
    
#elif USE_WM8731   // wm8731 in master or slave mode

    #define I2S_MASTER    1
        // the rPi is a horrible i2s master.
        // It is better with the wm831 as the master i2s device

    AudioInputI2S input;
    AudioOutputI2S output;

    #if I2S_MASTER
        AudioControlWM8731 control;
    #else
        AudioControlWM8731Slave control;
    #endif
    
#endif


AudioConnection  o0(input, 0, output, 0);
AudioConnection  o1(input, 1, output, 1);
#if USE_CS42448
    AudioConnection  o2(input, 2, output, 2);
    AudioConnection  o3(input, 3, output, 3);
    AudioConnection  o4(input, 4, output, 4);
    AudioConnection  o5(input, 5, output, 5);
#endif


void setup()
{
    printf("02-StereoPassThru::setup()\n");

    // The audio memory system can now be instantiated
    // with very large buffers ..
    
    AudioSystem::initialize(150);
    
    // The audio system now starts any i2s devices,
    // so you don't need to call control.enable().
   
    #if USE_WM8731
        // some devices do not have these controls
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
    #endif
    
    #if USE_WM8731 || USE_CS42448
        // ramp up the master volume over 1 second
        for (u16 i=0; i<=50; i++)
        {
            control.volume(((float)i) / 50.0);
            delay(20);
        }
    #endif

    #if USE_STGL5000
        control.setDefaults();
    #endif
    
    printf("02-StereoPassThru::setup() finished\n");
}



void loop()
{
}



