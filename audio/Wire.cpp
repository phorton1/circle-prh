#include "Wire.h"
#include <circle/logger.h>

#define I2C_CLOCK_SPEED     	100000


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
    // returns 0 on success
{
    size_t rslt = 0;
    if (m_len)
    {
        // printf("Wire endTransmission addr(0x%02x) len=%d data=0x%02x 0x%02x \n",m_addr,m_len,m_buf[0],m_buf[1]);
        rslt = m_pI2CMaster->Write(m_addr, m_buf, m_len);
        assert(rslt == m_len);
        // if (rslt != m_len)
        // {
        //     printf("got error result %d\n",m_len);
        // }
        return (rslt == m_len) ? 0 : 1;
    }
	return 0;
    
}


size_t CWire::write(u8 value)
{
    m_buf[m_len++] = value;
    return 1;
}
    

size_t CWire::read(u8 addr, u8 *buf, u8 len)
{
    size_t rslt = m_pI2CMaster->Read(addr, buf, len);
    assert(rslt == len);
    return rslt;
}

