
#include <audio\Audio.h>

// note that you must use 'master' control devices
// if you use 'slave' i2s devices.

#define USE_CS42448  1

#if USE_CS42448
    #define USE_MODULATION 0
    #define USE_MUSICAL_SCALE 1
#else
    #define USE_MODULATION 0
#endif



#if USE_MODULATION
    AudioSynthWaveformSine modulate;
    AudioSynthWaveformSineModulated input;
    AudioConnection  cc(modulate, 0, input, 0);
#elif USE_MUSICAL_SCALE
    AudioSynthWaveformSine input0;
    AudioSynthWaveformSine input1;
    AudioSynthWaveformSine input2;
    AudioSynthWaveformSine input3;
    AudioSynthWaveformSine input4;
    AudioSynthWaveformSine input5;
    AudioSynthWaveformSine input6;
    AudioSynthWaveformSine input7;
#else
    AudioSynthWaveformSine input;
#endif


#if USE_CS42448
    // Is starting randomly.  Synchronization is an issue.
    // Must learn exact correct startup sequence vis-a-vis DMA

    AudioOutputTDM output;
    AudioControlCS42448 control;
    AudioControlCS42448 *pControl;
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
    AudioConnection  c2(input2, 0, output, 2);
    AudioConnection  c3(input3, 0, output, 3);
    AudioConnection  c4(input4, 0, output, 4);
    AudioConnection  c5(input5, 0, output, 5);
    AudioConnection  c6(input6, 0, output, 6);
    AudioConnection  c7(input7, 0, output, 7);

#elif USE_CS42448

    AudioConnection  c0(input, 0, output, 0);    // white 1
    AudioConnection  c1(input, 0, output, 1);    // red   2
    // AudioConnection  c2(input, 0, output, 2);    // white 2  
    // AudioConnection  c3(input, 0, output, 3);    // red   3
    AudioConnection  c4(input, 0, output, 4);    // white 3
    AudioConnection  c5(input, 0, output, 5);    // red   4
    // AudioConnection  c6(input, 0, output, 6);    // white 4
    // AudioConnection  c7(input, 0, output, 7);    // red   1
    
#else
    AudioConnection  c0(input, 0, output, 0);
    AudioConnection  c1(input, 0, output, 1);
#endif


void setup()
{
    printf("01-HardwareTest::setup()\n");
    
    // initialize what you can before getting into the grunge.
    
    #if USE_MODULATION
        modulate.frequency(0.25);
        modulate.amplitude(0.50);
    #endif
    
    #if USE_MUSICAL_SCALE
        input0.frequency(261.63);
        input1.frequency(293.66);
        input2.frequency(329.63);
        input3.frequency(349.23);
        input4.frequency(392.00);
        input5.frequency(440.0);
        input6.frequency(493.88);
        input7.frequency(523.25);
    #else
        input.frequency(440.0);
    #endif
    
    // This order of operations is specific to the Octo
    // There is no "normal" way to do this, we are kludging
    // the static initialization to setup the bcm_pcm and
    // the code clocks can be closely related to the bcm
    // clocks, frame, etc.
    //
    // There is a question about how to do this well and still
    // retain the simplicity of static declarations.
    
    #if USE_CS42448
        pControl = &control;
        control.reset();    // the reset must be done before enable
    #endif
    
    control.enable();       // setup up the condec control bits ...    
    
    AudioMemory(20);        // Also setups and starts DMA for bcm_pcm devices
    
    // The bcm_pcm io devices require complicated setup that must
    // be performed after the kernel is initialized, so it cannot
    // be done on static objects like in the teensy. The call to
    // AudioMemory() not only allocates the memory, but starts the
    // devices.
    
    // control.setSampleRate(44100);
        // this needs to be done as soon after bcm_pcm::start() as
        // possible for reproducable frame synchronization, as
        // it starts the clocks that trigger the DMAs that were
        // just setup in "AudioMemory()" (bcm_pcm::start())
        
    #if USE_CS42448
        #if 0
            control.volume(1.0);

            // control.inputLevel(1,1.0);
            // control.inputLevel(2,1.0);

            // individual channels not implemented yet
            // control.volume(1,1.0);
            // control.volume(2,1.0);
            // control.volume(3,1.0);
            // control.volume(4,1.0);
            // control.volume(5,1.0);
            // control.volume(6,1.0);
            // control.volume(7,1.0);
        #endif
    #else
        control.volume(0.0);
            // we'll toggle it on and off in loop()
    #endif
    
    printf("01-HardwareTest::setup() finished\n");
}



void loop()
{
    #if !USE_CS42448
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
    #endif
}



