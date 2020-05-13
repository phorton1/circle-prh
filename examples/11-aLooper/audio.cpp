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



void setup()
{
    printf("11-aLooper::audio.cpp setup(%dx%d)\n",
           LOOPER_NUM_TRACKS,
           LOOPER_NUM_LAYERS);
        
    pLooper = new loopMachine();

    new AudioConnection(input,      0,      *pLooper,       0);
    new AudioConnection(input,      1,      *pLooper,       1);
    new AudioConnection(*pLooper,   0,      output,         0);
    new AudioConnection(*pLooper,   1,      output,         1);

    AudioSystem::initialize(200);

    pLooper->setControl(CONTROL_OUTPUT_GAIN,0);
    pLooper->setControl(CONTROL_INPUT_GAIN,0);
    delay(100);
    
    #if !USE_CS42448 && !USE_TEENSY_QUAD_SLAVE
        // some devices do not have these controls
        control.inputSelect(AUDIO_INPUT_LINEIN);
    #endif
    
    // set all volumes except output
    // and then ramp up the output volume
    
    for (int i=CONTROL_THRU_VOLUME; i<=CONTROL_OUTPUT_VOLUME; i++)
    {
        pLooper->setControl(i,pLooper->getControlDefault(i));
    }

    float default_out_val = pLooper->getControlDefault(CONTROL_OUTPUT_GAIN);
    float default_in_val = pLooper->getControlDefault(CONTROL_INPUT_GAIN);
    for (int j=0; j<50; j++)
    {
        u8 in_val = roundf(default_in_val * ((float)j)/50.00);
        u8 out_val = roundf(default_out_val * ((float)j)/50.00 );
        pLooper->setControl(CONTROL_INPUT_GAIN,in_val);
        pLooper->setControl(CONTROL_OUTPUT_GAIN,out_val);
        delay(20);
    }
    
    printf("11-aLooper::audio.cpp setup() finished\n");
    
}



void loop()
{
}



