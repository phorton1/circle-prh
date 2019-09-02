// 04-OctoTest.cpp
//                       
// This simplest test program just connects the 6 Octo Inputs
// to the first 6 Octo outputs as an isoldated test program.

#include <audio\Audio.h>

AudioInputTDM input;
AudioOutputTDM output;
AudioControlCS42448 control;


//-----------------------------------------------
// setup
//-----------------------------------------------

void setup()
{
    printf("04-OctoTest::setup() version 1.0\n");

    for (u16 i=0; i<6; i++)
    {
        new AudioConnection(input, i, output, i);
    }
    
    AudioSystem::initialize(150);

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
}



