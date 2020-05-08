// 11-pLooper

#include <audio\Audio.h>
#include "looper.h"

// You may define zero or one of the following.
// The default is wm8731.  You can also connect
// to the teensy audio card (STGL500) or a teensy
// running the AudioInputI2SQuad device.

#define USE_CS42448             1
#define USE_STGL5000            0
#define USE_TEENSY_QUAD_SLAVE   0

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


// The inputs are coupled to the outputs
// thru the looper

#if 0
    AudioConnection  o0(input, 0, output, 0);
    AudioConnection  o1(input, 1, output, 1);
    #if USE_CS42448
        AudioConnection  o2(input, 2, output, 2);
        AudioConnection  o3(input, 3, output, 3);
        AudioConnection  o4(input, 4, output, 4);
        AudioConnection  o5(input, 5, output, 5);
    #endif
#endif


// SINGLE GLOBAL STATIC INSTANCE

loopMachine *pLooper = 0;

// #define TEST_SINE_INPUT

#ifdef TEST_SINE_INPUT
    AudioSynthWaveformSine sine1;
    AudioSynthWaveformSine sine2;
#endif


void setup()
{
    printf("11-aLooper::audio.cpp setup()\n");

    // The audio memory system can now be instantiated
    // with very large buffers ..
    
    AudioSystem::initialize(150);
    
    // The audio system now starts any i2s devices,
    // so you don't need to call control.enable().
   
    #if !USE_CS42448 && !USE_TEENSY_QUAD_SLAVE
        // some devices do not have these controls
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
    
    // allocate and connect the looper here
    // it will be available to the public through an extern


    #if 1
    
        #if 1
            printf("allocating %dx%d looper\n",
               LOOPER_NUM_TRACKS,
               LOOPER_NUM_LAYERS);
        #endif
        
        pLooper = new loopMachine();
        
        #ifdef TEST_SINE_INPUT
            sine1.frequency(440);
            sine2.frequency(440);
        #endif

        #if 1
            #ifdef TEST_SINE_INPUT
                new AudioConnection(sine1,      0,      *pLooper,       0);
                new AudioConnection(sine2,      1,      *pLooper,       1);
                new AudioConnection(sine1,      0,      output,         0);
                new AudioConnection(sine2,      1,      output,         1);
            #else
                new AudioConnection(input,      0,      *pLooper,       0);
                new AudioConnection(input,      1,      *pLooper,       1);
                new AudioConnection(*pLooper,   0,      output,         0);
                new AudioConnection(*pLooper,   1,      output,         1);
            #endif
        #endif
        
        // this code should ONLY be included if the Looper object
        // is likewise configured to be 6x8.  This implementation
        // probably has limitations to stereo built in.
        
        #if 0 && USE_CS42448
            new AudioConnection(input,      2,    *pLooper,     2);
            new AudioConnection(input,      3,    *pLooper,     3);
            new AudioConnection(input,      4,    *pLooper,     4);
            new AudioConnection(input,      5,    *pLooper,     5);
            new AudioConnection(*pLooper,   2,    output,       2);
            new AudioConnection(*pLooper,   3,    output,       3);
            new AudioConnection(*pLooper,   4,    output,       4);
            new AudioConnection(*pLooper,   5,    output,       5);
            new AudioConnection(*pLooper,   6,    output,       6);
            new AudioConnection(*pLooper,   7,    output,       7);
        #endif    
    #endif
    
    // resort the streams in topological order
    
    AudioSystem::sortStreams();
    
    printf("11-aLooper::audio.cpp setup() finished\n");
    
}



void loop()
{
}



