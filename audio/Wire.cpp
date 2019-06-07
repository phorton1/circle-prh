#include "Wire.h"
#include <circle/logger.h>

#define I2C_CLOCK_SPEED     	100


#define log_name "wire"


CWire Wire;


CWire::CWire()
{
    m_len = 0;
    m_addr = 0;
    m_pI2CMaster = 0;
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
        m_pI2CMaster->SetClock(I2C_CLOCK_SPEED);
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
    m_len = 0;
    m_addr = addr;
}

int CWire::endTransmission()
{
    size_t rslt = 0;
    if (m_len)
    {
        rslt = m_pI2CMaster->Write(m_addr, m_buf, m_len);
        assert(rslt == m_len);
        m_len = 0;
    }
	return rslt;
    
}


size_t CWire::write(u8 value)
{
    m_buf[m_len++] = value;
    return 1;
}
    


