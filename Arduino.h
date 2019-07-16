// This header file is in the public domain.

#ifndef Arduino_h
#define Arduino_h

// #include "WProgram.h"
// #include "pins_arduino.h"

#include <circle/types.h>
#include <circle/memio.h>
#include <circle/util.h>
#include <circle/timer.h>
#include <circle/synchronize.h>

#include <stdint.h>
    // from gcc standard libraries, i *hope* this matches
    // circles definitions based on architecture
    

#define NULL    0

#define F_CPU   600000000

#define __enable_irq()      // EnableIRQs()
#define __disable_irq()	    // DisableIRQs()

#ifndef delay
#define delay(ms)   (CTimer::Get()->MsDelay(ms))
#endif

#define DMAMEM

#define abs(v)   (v > 0 ? v : -v)


#endif
