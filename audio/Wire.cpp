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
	delay(200);
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
        // LOG("Wire endTransmission addr(0x%02x) len=%d data=0x%02x 0x%02x",m_addr,m_len,m_buf[0],m_buf[1]);

        // m_pI2CMaster->SetClock(I2C_CLOCK_SPEED);
        rslt = m_pI2CMaster->Write(m_addr, m_buf, m_len);

		// assert(rslt == m_len);
		if (rslt != m_len)
		{
			LOG_ERROR("wrote(%d) expected(%d) bytes to m_addr(0x%08x)",rslt,m_len,m_addr);
			display_bytes("buffer",m_buf,m_len);
        }
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
	// never fails
    // assert(rslt == len);
	if (rslt != len)
	{
		LOG_ERROR("read(%d) expected(%d)",rslt,len);
	}
	return rslt;
}
