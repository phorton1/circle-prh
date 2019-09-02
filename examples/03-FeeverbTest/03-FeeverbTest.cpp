// 03-FreeverbTest.cpp
//
// This example program adds the Reverb/Freeverb objects.


#include <system/std_kernel.h>
#include <audio\Audio.h>


// you may define 0 or 1 of USE_REVERB and USE_FREEVERB
// USE_MIXER puts the output through a mixer first

#define USE_REVERB      0
#define USE_FREEVERB    1
#define USE_MIXER       1



#define USE_CS42448  0
    // default is AudioInjector Stereo wm8731 running the clocks

#if USE_CS42448
    AudioInputTDM input;
    AudioOutputTDM output;
    AudioControlCS42448 control;
#else
    #define I2S_MASTER   1
    AudioInputI2S input;
    AudioOutputI2S output;
    #if I2S_MASTER
        AudioControlWM8731 control;
    #else
        AudioControlWM8731Slave control;
    #endif
#endif



#if USE_FREEVERB
    AudioEffectFreeverbStereo reverb;
#elif USE_REVERB
    AudioEffectReverb reverb;
#endif


#if USE_MIXER
    AudioMixer4 mixer0;
    AudioMixer4 mixer1;
#endif


void setup()
{
    printf("03-FreeverbTest::setup()\n");
    
    #if USE_FREEVERB
        reverb.roomsize(0.6);
        reverb.damping(0.5);
    #elif USE_REVERB
        reverb.reverbTime(0.8);
    #endif

    AudioStream *bus0 = &input;
    AudioStream *bus1 = &input;
    u8 channel0 = 0;
    u8 channel1 = 1;

    #if USE_FREEVERB || USE_REVERB
        new AudioConnection(input, 0, reverb, 0);
        bus0 = &reverb;
        bus1 = &reverb;
        #if USE_REVERB
            channel1 = 0;
        #endif
    #endif
    
    #if USE_MIXER
        new AudioConnection(input, 0, mixer0, 0);
        new AudioConnection(input, 1, mixer1, 0);
        #if USE_FREEVERB || USE_REVERB
            new AudioConnection(*bus0, channel0, mixer0, 1);
            new AudioConnection(*bus1, channel1, mixer1, 1);
        #endif
        bus0 = &mixer0;
        bus1 = &mixer1;
        channel1 = 0;
    #endif
    
    new AudioConnection(*bus0, channel0, output, 0);
    new AudioConnection(*bus1, channel1, output, 1);
        
   
    // The audio memory system can now be instantiated
    // with very large buffers ..
    
    AudioSystem::initialize(150);
    
    // The audio system now starts any i2s devices,
    // so you don't need to call control.enable().
    
    #if !USE_CS42448
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
    #endif
        
    #if USE_MIXER
        mixer0.gain(0, 0.6);
        mixer0.gain(1, 0.1);
        mixer0.gain(2, 0.0);
        mixer0.gain(3, 0.0);
        mixer1.gain(0, 0.6);
        mixer1.gain(1, 0.1);
        mixer1.gain(2, 0.0);
        mixer1.gain(3, 0.0);
    #endif
    
    // ramp up the master volume over 1 second
    for (u16 i=0; i<=50; i++)
    {
        control.volume(((float)i) / 50.0);
        delay(20);
    }
    
    printf("03-FreeverbTest::setup() finished\n");
}



void loop()
{
}




