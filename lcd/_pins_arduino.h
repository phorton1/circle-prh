// _pins_arduino.h
//
// replaces pins_arduino.h for the addon/lcd projectr


#ifndef ___pins_arduino_h__
#define ___pins_arduino_h__

    #include <circle/types.h>
    #include <circle/gpiopin.h>
	#include <utils/myUtils.h>
    
    extern void init_pins_arduino();
    

    #define INPUT   0
    #define OUTPUT  1
    #define HIGH    1
    
    #define NULL    0
    #define PROGMEM
    
    extern void pinMode(u8 p, u8 v);
    extern void digitalWrite(u8 p, u8 v);
    int digitalRead(u8 p);
        // for touch screen
    
    // not supported
    
    #if 0
        extern u16  analogRead(u8 p);
        extern u8   digitalPinToBitMask(u8 p);
        extern u8   digitalPinToPort(u8 p);
        extern u8  *portOutputRegister(u8 port);
    #endif
    
    // we are currently assuming rPi GPIO 0..7 for 8 bit operations
    
    extern void  setReadDir();
    extern void  setWriteDir();
    extern void  write8(u8 v);
    extern u8   _read8();
    extern void _write8(u8 v);
    
    #define write8(v)   { _write8(v); WR_STROBE; }
    #define read8(v)    { RD_ACTIVE; DELAY7; *(&v) = _read8(); RD_IDLE; }

    extern void delay(u32 ms);  // in prhUtils.cpp


#endif
