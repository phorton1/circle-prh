// 11-aLooper

#include <audio\Audio.h>
#include "Looper.h"

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


// SINGLE GLOBAL STATIC INSTANCE

loopMachine *pLooper = 0;

#if USE_INPUT_AMP
    AudioAmplifier inputAmp1;
    AudioAmplifier inputAmp2;
#endif

#if USE_OUTPUT_MIXER
    AudioMixer4 outputMixer1;
    AudioMixer4 outputMixer2;
#endif


void setup()
{
    printf("11-aLooper::audio.cpp setup()\n");

    // The audio memory system can now be instantiated
    // with very large buffers ..
    
    // AudioSystem::initialize(200);
    
    // allocate and connect the looper here
    // it will be available to the public through an extern

    #if 1
        printf("allocating %dx%d looper\n",
           LOOPER_NUM_TRACKS,
           LOOPER_NUM_LAYERS);
    #endif
        
    pLooper = new loopMachine();
        

    #if USE_INPUT_AMP
        new AudioConnection(input,      0,      inputAmp1,      0);
        new AudioConnection(input,      1,      inputAmp2,      0);
        new AudioConnection(inputAmp1,  0,      *pLooper,       0);
        new AudioConnection(inputAmp2,  0,      *pLooper,       1);
    #else
        new AudioConnection(input,      0,      *pLooper,       0);
        new AudioConnection(input,      1,      *pLooper,       1);
    #endif    
    
    
    #if USE_OUTPUT_MIXER
        #if NO_THRU_LOOPER
            #if USE_INPUT_AMP
                new AudioConnection(inputAmp1,  0,      outputMixer1,       1);
                new AudioConnection(inputAmp2,  0,      outputMixer2,       1);
            #else
                new AudioConnection(input,      0,      outputMixer1,       1);
                new AudioConnection(input,      1,      outputMixer2,       1);
            #endif    
        #endif
    
        new AudioConnection(*pLooper,   0,      outputMixer1,   0);
        new AudioConnection(*pLooper,   1,      outputMixer2,   0);
        new AudioConnection(outputMixer1, 0,    output,         0);
        new AudioConnection(outputMixer2, 0,    output,         1);
    #else
        new AudioConnection(*pLooper,   0,      output,         0);
        new AudioConnection(*pLooper,   1,      output,         1);
    #endif            
    

    // AudioSystem::sortStreams();
   
    AudioSystem::initialize(200);

    // The audio system now starts any i2s devices,
    // so you don't need to call control.enable().
   
    #if !USE_CS42448 && !USE_TEENSY_QUAD_SLAVE
        // some devices do not have these controls
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
    #endif
    
    #if USE_CS42448
        control.inputLevel(0);   // -64 to +24 db, default = 0
    #endif

    #if USE_INPUT_AMP
        inputAmp1.gain(1.27);
        inputAmp2.gain(1.27);
    #endif
    
    #if USE_OUTPUT_MIXER
        outputMixer1.gain(0,1.0);
        outputMixer1.gain(1,1.0);
        outputMixer2.gain(0,1.0);
        outputMixer2.gain(1,1.0);
    #endif

    
    #if !USE_TEENSY_QUAD_SLAVE
        // ramp up the master volume over 1 second
        for (u16 i=0; i<=50; i++)
        {
            control.volume(((float)i) / 50.0);
            delay(20);
        }
    #endif
    
    printf("11-aLooper::audio.cpp setup() finished\n");
    
}



void loop()
{
}



