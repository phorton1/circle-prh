

#include "Wire.h"
#include <circle/logger.h>

#define log_name "wire"


CWire Wire;


CWire::CWire()
{
    m_pI2CMaster = 0;
    m_addr = 0;
}

CWire::~CWire()
{
    end();
}


void CWire::begin()
{
    LOG("begin()",0);
    
    if (!m_pI2CMaster)
    {
        LOG("creating i2cMaster",0);
        CMachineInfo *mi = CMachineInfo::Get();
        assert(mi);
        m_pI2CMaster = new CI2CMaster(mi->GetDevice(DeviceI2CMaster));
        assert(m_pI2CMaster);
    }
}

void CWire::end()
{
    if (m_pI2CMaster)
        delete m_pI2CMaster;
    m_pI2CMaster = 0;
}


void CWire::beginTransmission(u8 addr)
{
    m_addr = addr;
}

void CWire::endTransmission()
{
    
}


size_t CWire::write(const uint8_t *buf, size_t len)
{
    assert(m_pI2CMaster);
    assert(m_addr);
    size_t rslt = m_pI2CMaster->Write(m_addr, buf, len)        ;
    assert(rslt == len);
    return rslt;
}

size_t CWire::write(u8 value)
{
    return write(&value,1);
}
    


