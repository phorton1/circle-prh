#ifndef __WIRE_H__
#define __WIRE_H__

#include <assert.h>
#include <Arduino.h>
#include <circle/i2cmaster.h>
#include <circle/machineinfo.h>

#define MAX_WIRE_BYTES   32
    // we have to buffer the bytes sent to us, and then transmit them
    // all on endTransmission in order to act like the arduino

class CWire
{
public:

    CWire();
    ~CWire();
    
    void begin();
    void end();
    
    void beginTransmission(u8 addr);
    int endTransmission();
    
	size_t write(u8 value);
    
private:
    
    u8 m_len;
    u8 m_addr;
    u8 m_buf[MAX_WIRE_BYTES];
    
    CI2CMaster *m_pI2CMaster;
    
};


extern CWire Wire;


#endif

