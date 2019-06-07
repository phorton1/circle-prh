
#include <audio\Audio.h>

// note that you must use 'master' control devices
// if you use 'slave' i2s devices.

#define I2S_MASTER  1

AudioSynthWaveformSine input;

#if I2S_MASTER
    AudioOutputI2S output;
    AudioControlWM8731 control;
#else
    AudioOutputI2Sslave output;
    AudioControlWM8731master control;
#endif

AudioConnection  c0(input, 0, output, 0);
AudioConnection  c1(input, 0, output, 1);


void setup()
{
    printf("01-HardwareTest::setup()\n");
    
    input.frequency(440.0);

    AudioMemory(20);

    control.enable();
    control.volume(0.0);

    printf("01-HardwareTest::setup() finished\n");
}



void loop()
{
    static int counter = 0;
    if (counter++ < 6)
    {
        control.volume(counter & 1 ? 0.6 : 0.00);
        delay(500);
    }
    else
    {
        counter = 0;
        delay(2000);
    }
}



