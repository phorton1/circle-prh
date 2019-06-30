
#include "_pins_arduino.h"
#include "mcu_8bit_magic.h"
#include <assert.h>
#include <circle/util.h>
#include <circle/logger.h>

#define log_name "pins_arduino"


#define PIN_DATA0   0
#define DATA_MASK   0xff
    // we are currently assuming GPIO pins 0..7 constitute
    // the data byte.  Would need shifts otherwise.
    
    
#define MAX_RPI_PINS   54

CGPIOPin *pin_data[MAX_RPI_PINS];


void init_pins_arduino()
{
    LOG("init_pins_arduino()",0);
    memset(pin_data,0,MAX_RPI_PINS * sizeof(void *));
	for (int i=0; i<8; i++)
	{
		pin_data[i] = new CGPIOPin(PIN_DATA0 + i,GPIOModeOutput);
        digitalWrite(i,LOW);
	}
}





void pinMode(u8 p, u8 v)
{
    // LOG("pinMode(%d,%d)",p,v);
    if (!pin_data[p])
        pin_data[p] = new CGPIOPin(p, v ?
            GPIOModeOutput :
            GPIOModeInput );
    else
        pin_data[p]->SetMode(v ?
            GPIOModeOutput :
            GPIOModeInput,
            true);
}


void digitalWrite(u8 p, u8 v)
{
    // LOG("digitalWrite(%d,%d)",p,v);
    assert(pin_data[p]);
    pin_data[p]->Write(v);
}

int digitalRead(u8 p)
{
    // LOG("digitalWrite(%d,%d)",p,v);
    assert(pin_data[p]);
    return pin_data[p]->Read();
}


#if 0
    u16 analogRead(u8 p)
    {
        assert(0 && "analogRead() not supported on rPi");
        return 0;
    }
    u8 digitalPinToBitMask(u8 p)
    {
        assert(0 && "digitalPinToBitMask() not supported");
        return 0xff;
    }
    u8 digitalPinToPort(u8 p)
    {
        assert(0 && "digitalPinToPort() not supported");
        return 0xff;
    }
    u8 *portOutputRegister(u8 port)
    {
        assert(0 && "portOutputRegister() not supported");
        return 0;
    }
#endif


void setReadDir()
{
    // LOG("setReadDir()",0);
	for (int i=0; i<8; i++)
	{
		pin_data[i]->SetMode( GPIOModeInput ); // PullDown );
	}	    
}

void setWriteDir()
{
    // LOG("setWriteDir()",0);
	for (int i=0; i<8; i++)
	{
		pin_data[i]->SetMode( GPIOModeOutput );
	}	    
}

void _write8(u8 v)
{
    // LOG("write8(0x%02x)",v);
	CGPIOPin::WriteAll(v,DATA_MASK);
}

u8  _read8()
{
	u32 data = CGPIOPin::ReadAll();
    // LOG("read8() got 0x%02x",data & DATA_MASK);
	return (data & DATA_MASK);}

