
#include <audio\Audio.h>

// note that you must use 'master' control devices
// if you use 'slave' i2s devices.

#define USE_CS42448  1

#if USE_CS42448
    #define USE_MODULATION 1
        // distinctive sound helps with channel debugging
#else
    #define USE_MODULATION 0
#endif



#if USE_MODULATION
    AudioSynthWaveformSine modulate;
    AudioSynthWaveformSineModulated input;
    AudioConnection  cc(modulate, 0, input, 0);
#else
    AudioSynthWaveformSine input;
#endif


#if USE_CS42448
    // Is starting randomly.  Synchronization is an issue.
    // Must learn exact correct startup sequence vis-a-vis DMA

    AudioOutputTDM output;
    AudioControlCS42448 control;

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


#if USE_CS42448

    // At this point I can fairly reliably get noise out of White#1
    // for output 0, and occasionally get the sound on reboots
    
    // I think the interleaving is 0,4,  1,5,  2,6,  3,7
    // where 0=white #1 and 1=red #1
    
    // as when I can see the signal crossing the FCLK and it shows
    // up in paairs that are always next to each other above (i.e. 4-1
    // or 1-5)
    
    AudioConnection  c0(input, 0, output, 0);

    #if 0    
        AudioConnection  c1(input, 0, output, 1);
        AudioConnection  c2(input, 0, output, 2);
        AudioConnection  c3(input, 0, output, 3);
        AudioConnection  c4(input, 0, output, 4);
        AudioConnection  c5(input, 0, output, 5);
        AudioConnection  c6(input, 0, output, 6);
        AudioConnection  c7(input, 0, output, 7);
    #endif
    
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
    
    input.frequency(440.0);

    // This order of operations is specific to the Octo
    // There is no "normal" way to do this, we are kludging
    // the static initialization to setup the bcm_pcm and
    // the code clocks can be closely related to the bcm
    // clocks, frame, etc.
    //
    // There is a question about how to do this well and still
    // retain the simplicity of static declarations.
    
    #if USE_CS42448
        control.reset();    // the reset must be done before enable
    #endif
    
    control.enable();       // setup up the condec control bits ...    
    
    AudioMemory(20);        // Also setups and starts DMA for bcm_pcm devices
    
    // The bcm_pcm io devices require complicated setup that must
    // be performed after the kernel is initialized, so it cannot
    // be done on static objects like in the teensy. The call to
    // AudioMemory() not only allocates the memory, but starts the
    // devices.
    
    control.setSampleRate(44100);
        // this needs to be done as soon after bcm_pcm::start() as
        // possible for reproducable frame synchronization, as
        // it starts the clocks that trigger the DMAs that were
        // just setup in "AudioMemory()" (bcm_pcm::start())
        
    #if USE_CS42448
        control.volume(1.0);
        // individual channels not implemented yet
        // control.volume(1,1.0);
        // control.volume(2,1.0);
        // control.volume(3,1.0);
        // control.volume(4,1.0);
        // control.volume(5,1.0);
        // control.volume(6,1.0);
        // control.volume(7,1.0);
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



