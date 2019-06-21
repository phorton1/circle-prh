
#include <audio\Audio.h>

// you may define 0 or 1 of USE_REVERB and USE_FREEVERB
// USE_MIXER puts the output through a mixer first

#define USE_REVERB      0
#define USE_FREEVERB    0
#define USE_MIXER       1
    

#define USE_CS42448  1

#if USE_CS42448
    AudioInputTDM input;
    AudioOutputTDM output;
    AudioControlCS42448 control;
#else
    #define I2S_MASTER   0
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


#if USE_FREEVERB
    AudioEffectFreeverbStereo reverb;
    #define SECOND_CHANNEL  1
    AudioConnection c0(input, 0, reverb, 0);
    AudioConnection c1(input, 1, reverb, 1);
#elif USE_REVERB
    AudioEffectReverb reverb;
    AudioConnection c0(input, 0, reverb, 0);
    #define SECOND_CHANNEL  0
#else
    #define reverb input
    #define SECOND_CHANNEL  1
#endif


#if USE_MIXER
    AudioMixer4 mixer1;
    AudioMixer4 mixer2;
    AudioConnection c2(input,  0, mixer1, 0);
    AudioConnection c3(input,  1, mixer2, 0);
    AudioConnection c4(reverb, 0, mixer1, 1);
    AudioConnection c5(reverb, SECOND_CHANNEL, mixer2, 1);
    AudioConnection c6(mixer1, 0, output, 0);
    AudioConnection c7(mixer2, 0, output, 1);
#else
    AudioConnection c2(reverb, 0, output, 0);
    AudioConnection c3(reverb, SECOND_CHANNEL, output, 1);
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

    control.enable();
        
    AudioMemory(100);

    #if !USE_CS42448
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
    #endif

    control.volume(1.0);
        
    #if USE_MIXER
        mixer1.gain(0, 0.01);
        mixer1.gain(1, 0.0);
        mixer1.gain(2, 0.0);
        mixer1.gain(3, 0.0);
        mixer2.gain(0, 0.0);
        mixer2.gain(1, 0.0);
        mixer2.gain(2, 0.0);
        mixer2.gain(3, 0.0);
    #endif
    
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
    #elif 1
        // If I instead go back and assume there will be a scheduler,
        // or at least check if there is one, and implement delay()
        // in terms of CScheduler::sleep(), then I can take all the
        // time I want ..

        CScheduler::Get()->MsSleep(2000);

    #elif 1
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




