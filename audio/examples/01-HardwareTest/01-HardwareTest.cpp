
#include <audio\Audio.h>

// note that you must use 'master' control devices
// if you use 'slave' i2s devices.

#define USE_CS42448             0
#define USE_TEENSY_QUAD_SLAVE   1


#if USE_CS42448
    #define USE_MUSICAL_SCALE   1
    #define USE_MODULATION      0
    #define TOGGLE_SOUND        1
    #define VOLUME_LEVEL        0.6
#else
    #define USE_MUSICAL_SCALE   0
    #define USE_MODULATION      0
    #define TOGGLE_SOUND        1
    #define VOLUME_LEVEL        0.6
#endif



#if USE_MUSICAL_SCALE
    AudioSynthWaveformSine input0;
    AudioSynthWaveformSine input1;
    #if USE_CS42448
        AudioSynthWaveformSine input2;
        AudioSynthWaveformSine input3;
        AudioSynthWaveformSine input4;
        AudioSynthWaveformSine input5;
        AudioSynthWaveformSine input6;
        AudioSynthWaveformSine input7;
    #endif
#elif USE_MODULATION
    AudioSynthWaveformSine modulate;
    AudioSynthWaveformSineModulated input;
    AudioConnection  cc(modulate, 0, input, 0);
#else
    AudioSynthWaveformSine input;
#endif


#if USE_CS42448
    AudioOutputTDM output;
    AudioControlCS42448 control;
#elif USE_TEENSY_QUAD_SLAVE
    // AudioInputTeensyQuad   input;
    AudioOutputTeensyQuad  output;
#else

    #define I2S_MASTER   1

    #if I2S_MASTER
        AudioOutputI2S output;
        AudioControlWM8731 control;
    #else
        AudioOutputI2Sslave output;
        AudioControlWM8731master control;
    #endif
#endif


#if USE_MUSICAL_SCALE
    AudioConnection  c0(input0, 0, output, 0);
    AudioConnection  c1(input1, 0, output, 1);
    #if USE_CS42448
        AudioConnection  c2(input2, 0, output, 2);
        AudioConnection  c3(input3, 0, output, 3);
        AudioConnection  c4(input4, 0, output, 4);
        AudioConnection  c5(input5, 0, output, 5);
        AudioConnection  c6(input6, 0, output, 6);
        AudioConnection  c7(input7, 0, output, 7);
    #endif
#elif USE_CS42448
    AudioConnection  c0(input, 0, output, 0);    
    AudioConnection  c1(input, 0, output, 1);    
    AudioConnection  c2(input, 0, output, 2);    
    AudioConnection  c3(input, 0, output, 3);    
    AudioConnection  c4(input, 0, output, 4);    
    AudioConnection  c5(input, 0, output, 5);    
    AudioConnection  c6(input, 0, output, 6);    
    AudioConnection  c7(input, 0, output, 7);    
#else
    AudioConnection  c0(input, 0, output, 0);
    AudioConnection  c1(input, 0, output, 1);
#endif


void setup()
{
    printf("01-HardwareTest::setup() version 1.0\n");
    
    // initialize what you can before getting into the grunge.
    
    #if USE_MODULATION
        modulate.frequency(0.25);
        modulate.amplitude(0.50);
    #endif
    
    #if USE_MUSICAL_SCALE
        input0.frequency(261.63);
        input1.frequency(293.66);
        #if USE_CS42448
            input2.frequency(329.63);
            input3.frequency(349.23);
            input4.frequency(392.00);
            input5.frequency(440.0);
            input6.frequency(493.88);
            input7.frequency(523.25);
        #endif
    #else
        input.frequency(440.0);
    #endif
    
    #if !USE_TEENSY_QUAD_SLAVE
        control.enable();       // setup up the condec control bits ...    
    #endif
    
    AudioMemory(20);        // Also setups and starts DMA for bcm_pcm devices
    
    // The bcm_pcm io devices require complicated setup that must
    // be performed after the kernel is initialized, so it cannot
    // be done on static objects like in the teensy. The call to
    // AudioMemory() not only allocates the memory, but starts the
    // devices.  
        
    #if USE_CS42448
        control.setSampleRate(44100);
            // this needs to be done as soon after bcm_pcm::start() as
            // possible for reproducable frame synchronization, as
            // it starts the clocks that trigger the DMAs that were
            // just setup in "AudioMemory()" (bcm_pcm::start())
    #endif
    
    #if !USE_TEENSY_QUAD_SLAVE
        control.volume(VOLUME_LEVEL);
    #endif

    printf("01-HardwareTest::setup() finished\n");
}



void loop()
{
    #if TOGGLE_SOUND
        static int counter = 0;
        if (counter++ < 6)
        {
            #if USE_MUSICAL_SCALE
                input0.amplitude(counter & 1 ? 0.25 : 0.00);
                input1.amplitude(counter & 1 ? 0.25 : 0.00);
                #if USE_CS42448
                    input2.amplitude(counter & 1 ? 0.25 : 0.00);
                    input3.amplitude(counter & 1 ? 0.25 : 0.00);
                    input4.amplitude(counter & 1 ? 0.25 : 0.00);
                    input5.amplitude(counter & 1 ? 0.25 : 0.00);
                    input6.amplitude(counter & 1 ? 0.25 : 0.00);
                    input7.amplitude(counter & 1 ? 0.25 : 0.00);
                #endif            
            #else
                input.amplitude(counter & 1 ? 0.25 : 0.00);
            #endif
            
            delay(500);
        }
        else
        {
            counter = 0;
            delay(2000);
        }
    #endif
}



