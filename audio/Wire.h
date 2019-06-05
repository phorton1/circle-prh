#ifndef __WIRE_H__
#define __WIRE_H__

#include <assert.h>
#include <Arduino.h>
#include <circle/i2cmaster.h>
#include <circle/machineinfo.h>

class CWire
{
public:

    CWire();
    ~CWire();
    
    void begin();
    void end();
    
    void beginTransmission(u8 addr);
    void endTransmission();
    
	size_t write(u8 value);
    size_t write(const uint8_t *buf, size_t len);
    
private:
    
    CI2CMaster *m_pI2CMaster;
    u8 m_addr;
    
};


extern CWire Wire;


#endif

