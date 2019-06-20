
#include <audio\Audio.h>

// You may only define zero or one of the following
// the default is wm8731

#define USE_CS42448             0
#define USE_STGL5000            0
#define USE_TEENSY_QUAD_SLAVE   0


#if USE_CS42448

    // Octo is always the master
    
    AudioInputTDM input;
    AudioOutputTDM output;
    AudioControlCS42448 control;

#elif USE_TEENSY_QUAD_SLAVE

    // Use 16 bit is2s channel to the 2nd i2s device on the teensy.
    // There is no "control" object at this time. You send and
    // recieve streams on channels 3 and 4 of the generic teensy
    // quad device.
    //
    // There is a very simple associated teensy INO sketch
    //
    //      teensyPiQuadTest.ino
    //
    // that just sends the quad inputs 0 and 1 TO the quad
    // outputs 3 and 4, and then takes the quad inputs 3 and 4 and
    // sends them out to the outputs 0 and 1 (the headphones).
    //
    // You hook up the same BLCK and FCLK (LRCLK) wires:
    //
    //      BCLK rpi_gpio(18) --> teensy_gpio(11)
    //      FCLK rpi_gpio(19) --> teensy_gpio(23)
    //
    // But send and receive on the 2nd teensy i2s pins.
    //
    //      RPI_TX_gpio(20) --> teensy_RX_gpio(30)  // normally 13
    //      RPI_TX_gpio(21) --> teensy_RX_gpio(15)  // normally 22
    //
    // Unfortunately, you can't easily get to the underside pin30
    // on a teensy3.2 ... hmmmm.
    
    AudioInputTeensyQuad   input;
    AudioOutputTeensyQuad  output;

#elif USE_STGL5000  // only in slave mode

    // #if I2S_MASTER
    // the rpi cannot be a master to an sgtl5000.
    // the sgtl5000 requires 3 clocks and the rpi can only generate 2
    //
    // AudioInputI2S input;
    // AudioOutputI2S output;
    // AudioControlSGTL5000 control;
    // #else
    
    AudioInputI2Sslave input;
    AudioOutputI2Sslave output;
    AudioControlSGTL5000master control;

    // #endif
    
#else   // wm8731 in master or slave mode

    #define I2S_MASTER    0

    // Is the BCM_PCM the master or slave device?
    // Of the devices so far, only the wm8731 can be a master.
    // Paul usually thinks the teensy is the master device,
    // and adds suffixes to the class names when it's not, but
    // the rpi generally behaves better as the i2s slave than
    // as the master.
    
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


AudioConnection  c0(input, 0, output, 0);
AudioConnection  c1(input, 0, output, 1);



void setup()
{
    printf("02-StereoPassThru::setup()\n");

    #if !USE_TEENSY_QUAD_SLAVE
        control.enable();
    #endif
    
    AudioMemory(80);
    
    #if !USE_CS42448 && !USE_TEENSY_QUAD_SLAVE
        control.inputSelect(AUDIO_INPUT_LINEIN);
        control.inputLevel(1.0);
    #endif
    
    #if !USE_TEENSY_QUAD_SLAVE
        control.volume(1.0);
    #endif
    
    printf("02-StereoPassThru::setup() finished\n");
}



void loop()
{
}



