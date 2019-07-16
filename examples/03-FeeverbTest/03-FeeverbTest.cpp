// requires USE_AUDIO_SYSTEM=1 to be set in std_kernl.h

#include <system/std_kernel.h>
#include <audio\Audio.h>

// including an application requries USE_UI_SYSTEM to be set in std_kernel.h,
// which in turn requires you to link in a wsApplication::Create() method.
// The wsNullApp.h allows you to create headless apps, while still linking
// the UI system with no windows.

// I am considering writing a linking loader.
// But for now, I create a new subtree with apps/recorder that
// is a linkable "application"


#if USE_UI_SYSTEM
    #include <ws/wsNullApp.h>
#endif


// you may define 0 or 1 of USE_REVERB and USE_FREEVERB
// USE_MIXER puts the output through a mixer first

#define USE_REVERB      0
#define USE_FREEVERB    1
#define USE_MIXER       1
#define USE_RECORDER    1

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

#if USE_RECORDER
    AudioRecorder recorder;
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
    
    #if USE_RECORDER
        new AudioConnection(*bus0, channel0, recorder, 0);
        new AudioConnection(*bus1, channel1, recorder, 1);
        #if USE_FREEVERB
            new AudioConnection(reverb, 0, recorder, 2);
            new AudioConnection(reverb, 1, recorder, 3);
        #elif USE_REVERB
            new AudioConnection(reverb, 0, recorder, 2);
        #endif
        bus0 = &recorder;
        bus1 = &recorder;
        channel1 = 1;
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



#include <circle/sched/scheduler.h>
    // for testing different update() strategies

void loop()
{
    // do some work in the loop
    // one way or the other the client has to be aware
    // and either keep their code in loop() short, or
    // call Yield appropriately.
    
    #if 0
        delay(3);
            // delay(), which calls Timer::MsDelay(), does not yield,
            // and so you get overflows and artifcats starting at 3ms
    #elif 0
        // If I instead go back and assume there will be a scheduler,
        // or at least check if there is one, and implement delay()
        // in terms of CScheduler::sleep(), then I can take all the
        // time I want ..

        CScheduler::Get()->MsSleep(2000);

    #elif 0
        // at about 100,000 times through this loop
        // I start getting overflows and noise artifacts
        // with a straight thru config on the Octo, which
        // uses very little CPU time
        //
        // by adding Yield calls it can go on forever if you want ..
        
        volatile u32 i,j;
        for (i=0; i<100000; i++)
        {
            if (i) j = i;
            if (j) i = i;
            #if 1
                CScheduler::Get()->Yield();
            #endif
        }
    #endif
}




