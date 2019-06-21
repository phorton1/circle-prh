
#include <audio\Audio.h>



u8 channel_map[] = {
    0,  0,  0,
    1,  1,  0, 
    2,  2,  0,
    3,  3,  0,
    4,  4,  0,
    5,  5,  0,
    6,  6,  0 };


u8 sine_map[] = {
    0,  0,  3,  
    1,  1,  3,
    2,  2,  3,
    3,  3,  3,
    4,  4,  3,
    5,  5,  3,
    6,  6,  3,
    7,  7,  3, };
    

AudioInputTDM           input;
AudioSynthWaveformSine  sine[8];
AudioMixer4             mixer[8];
AudioOutputTDM          output;
AudioControlCS42448     control;


void setup()
{
    printf("04-OctoTest::setup() version 1.0\n");

    sine[0].frequency(261.63);
    sine[1].frequency(293.66);
    sine[2].frequency(329.63);
    sine[3].frequency(349.23);
    sine[4].frequency(392.00);
    sine[5].frequency(440.0);
    sine[6].frequency(493.88);
    sine[7].frequency(523.25);
    
    // setup the audio connections
    
    u8 *p = channel_map;
    for (u16 i=0; i<sizeof(channel_map)/3; i++)
    {
        new AudioConnection(input, p[0],  mixer[p[1]], p[2]);
        p += 3;
    }
    
    p = sine_map;
    for (u16 i=0; i<sizeof(sine_map)/3; i++)
    {
        new AudioConnection(sine[p[0]],  0, mixer[p[1]], p[2]);
        p += 3;
    }
    
    for (u16 i=0; i<8; i++)
    {
        new AudioConnection(mixer[i], 0, output, i);
    }
    
    control.enable();        // setup up the condec control bits ...    
    AudioMemory(100);        // Also setups and starts DMA for bcm_pcm devices
    
    // zero the volumes
    
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
        mixer[p[1]].gain(p[2],0.005);
        p += 3;
    }
    
    p = sine_map;
    for (u16 i=0; i<sizeof(sine_map)/3; i++)
    {
        mixer[p[1]].gain(p[2],0.05);
        p += 3;
    }
    
    // ramp ump the master volume
    
    for (u16 i=0; i<100; i++)
    {
        control.volume(((float)i) / 100);
        delay(4);
    }

    printf("04-OctoTest::setup() finished\n");
}



void loop()
{
}



