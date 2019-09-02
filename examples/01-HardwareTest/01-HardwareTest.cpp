// 01-HardwareTest.cpp
//
// This, the initial audio example, merely makes a tone that
// comes out of the audio device.  It does not make use of any
// input devices.

#include <audio\Audio.h>

// By default, this test program uses the WM8731 AudioInjector Stereo
// audio card set to the Master BCLK/FCLK device.  You may define ONE
// of the following two defines to use the CS42448 AudioInjector Octo
// audio card, or to interface to a Teensy running the AudioInputI2SQuad
// device where channels 3/4 are hooked up as I/O channels to the rPi.

#define USE_CS42448             0
#define USE_TEENSY_QUAD_SLAVE   0

// These defines let you choose various ways the sound
// comes out.  On the CS42448, for instance, it can be
// useful to have each channel output a different note
// on the 8 tone musical scale with USE_MUSICAL_SCALE.
// USE_MODULATION creates a distinctive sound that can
// be differentiated from a static backround sine wave.
// TOGGLE_SOUND sends out three half second "beeps" with
// a 1.5 second interval between them, and the default
// VOLUME_LEVEL is used to generate a comfortable sound
// for my setup.

#if USE_CS42448
    #define USE_MUSICAL_SCALE   1
    #define USE_MODULATION      0
    #define TOGGLE_SOUND        1
    #define VOLUME_LEVEL        0.5
#else
    #define USE_MUSICAL_SCALE   0
    #define USE_MODULATION      0
    #define TOGGLE_SOUND        1
    #define VOLUME_LEVEL        0.5
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


// note that you must use 'master' control devices
// if you use 'slave' i2s devices.  In general, the
// rPi is horrible as the clock master, so all of these
// examples, by default, use the attached device as
// the i2S master ..


#if USE_CS42448
    AudioOutputTDM output;
    AudioControlCS42448 control;
#elif USE_TEENSY_QUAD_SLAVE
    // AudioInputTeensyQuad   input;
    AudioOutputTeensyQuad  output;
#else

    #define I2S_MASTER   1
        // the rPi is a horrible i2s master.
        // It is better with the wm831 as the master i2s device

    AudioOutputI2S output;

    #if I2S_MASTER
        AudioControlWM8731 control;
    #else
        AudioControlWM8731Slave control;
    #endif
#endif


// This early test program explicitly declares static
// connection objects.  Later these will prove problematic
// for soft-routable connections, as you cannot properly
// "delete" them.   

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
    
    // The audio memory system can now be instantiated
    // with very large buffers ..
    
    AudioSystem::initialize(150);
    
    // The audio system now starts any i2s devices,
    // so you don't need to call control.enable().
    
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



