
#include <audio\Audio.h>

#define WITH_MIXER 1
#define WITH_SINE  0        // requires WITH_MIXER
#define WITH_PROBE 1

#if WITH_PROBE
    #include "../statusScreen.h"
    #include <circle/sched/scheduler.h>
#endif

u8 channel_map[] = {
    0,  0,  0,
    1,  1,  0, 
    2,  2,  0,
    3,  3,  0,
    4,  4,  0,
    5,  5,  0 };


u8 sine_map[] = {
    0,  0,  3,  
    1,  1,  3,
    2,  2,  3,
    3,  3,  3,
    4,  4,  3,
    5,  5,  3,
    6,  6,  3,
    7,  7,  3, };
    

AudioInputTDM input;

#if WITH_SINE
    AudioSynthWaveformSine sine[8];
#endif

#if WITH_MIXER
    AudioMixer4 mixer[8];
#endif

AudioOutputTDM output;
AudioControlCS42448 control;

#if WITH_PROBE
    AudioProbe probe(1000,0);
    boolean started = 0;
#endif


//-----------------------------------------------
// setup
//-----------------------------------------------

void setup()
{
    printf("04-OctoTest::setup() version 1.0\n");

#if WITH_SINE
    sine[0].frequency(261.63);
    sine[1].frequency(293.66);
    sine[2].frequency(329.63);
    sine[3].frequency(349.23);
    sine[4].frequency(392.00);
    sine[5].frequency(440.0);
    sine[6].frequency(493.88);
    sine[7].frequency(523.25);
#endif

    // setup the audio connections
    
#if WITH_SINE || WITH_MIXER
    u8 *p;
#endif


#if WITH_MIXER    
    p = channel_map;
    for (u16 i=0; i<sizeof(channel_map)/3; i++)
    {
        new AudioConnection(input, p[0],  mixer[p[1]], p[2]);
        p += 3;
    }
#endif

#if WITH_SINE
    p = sine_map;
    for (u16 i=0; i<sizeof(sine_map)/3; i++)
    {
        new AudioConnection(sine[p[0]],  0, mixer[p[1]], p[2]);
        p += 3;
    }
#endif

    for (u16 i=0; i<8; i++)
    {
        #if WITH_MIXER
            new AudioConnection(mixer[i], 0, output, i);
        #else
            new AudioConnection(input, i, output, i);
        #endif
    }
    
    control.enable();        // setup up the condec control bits ...    
    AudioMemory(100);        // Also setups and starts DMA for bcm_pcm devices
    
    // zero the volumes
    
#if WITH_MIXER    
    for (u16 i=0; i<8; i++)
    {
        for (u16 j=0; j<4; j++)
        {
            mixer[i].gain(j, 0.0);
        }
    }

    p = channel_map;
    for (u16 i=0; i<sizeof(channel_map)/3; i++)
    {
        mixer[p[1]].gain(p[2],1.0);
        p += 3;
    }
#endif


#if WITH_SINE
    p = sine_map;
    for (u16 i=0; i<sizeof(sine_map)/3; i++)
    {
        mixer[p[1]].gain(p[2],0.05);
        p += 3;
    }
#endif

#if WITH_PROBE
    new AudioConnection(input, 0, probe, 0);
    #if WITH_MIXER
        new AudioConnection(mixer[0],0,probe,1);
    #else
        new AudioConnection(input, 1, probe, 1);
    #endif
#endif

    // ramp up the master volume over 1 second
    
    for (u16 i=0; i<=50; i++)
    {
        control.volume(((float)i) / 50.0);
        delay(20);
    }

    printf("04-OctoTest::setup() finished\n");
}



void loop()
{
    #if WITH_PROBE
        if (!started)
        {
            started = 1;
            if (0)
            {
                printf("starting probe in 2 seconds\n");
                CScheduler::Get()->MsSleep(2000);
            }
            printf("starting probe ..\n");
            statusScreen::get()->pause();
            probe.start();
            if (0)
            {
                CScheduler::Get()->MsSleep(5000);
                printf("restarting screen ..\n");
                probe.stop();
                statusScreen::get()->resume();
            }
        }
    #endif
}



