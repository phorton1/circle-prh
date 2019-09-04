//
// uartTransportBase.h
//
// base class for transportUart and transportMiniUart
// does the common re-assembly of hci packets, and
// implements generic sendHCICommand() and sendHCIData()
// methods.

#ifndef _transport_base_h
#define _transport_base_h

#include "_hci_defs.h"
#include <circle/device.h>
#include <circle/bt/bttransportlayer.h>
#include <circle/types.h>


#define UART_TRANSPORT_BUFFER_SIZE  0xFFFF
    // quick and dirty solution to packets > HCI_MAX_PACKET_SIZE
    //
    // The UART can buffer upto a full 64k sized ACL packet.
    // I don't really like having this huge static buffer, but it's
    // not that big considering we are on a 512MB+ machine.
    //
    // I don't really like losing the error checking assertions
    // on hci commands and events, but this, or anbother rework
    // of everything, was needed for ACL packets once I got to
    // the sDP layer.


class uartTransportBase :
    public CDevice,
    public btTransportBase
{
public:
    
	uartTransportBase();
	~uartTransportBase(void);

    virtual boolean isUartTransport()  { return true; }
	virtual void registerPacketHandler(void *pHCILayer, TBTHCIEventHandler *pHandler);
        // overrides from btTransportBase
        
	virtual boolean SendHCIData(const void *pBuffer, unsigned length);
	virtual boolean SendHCICommand(const void *pBuffer, unsigned length);
        // overrides from btTransportBase
        // casing kept compatible with circle bt usb device
    
	virtual void initialize(unsigned nBaudrate = 115200) = 0;
        // additional pure virtual method

        
protected:
    
	virtual void Write(u8 nChar) = 0;
        // additional pure virtual method
    void Receive(u8 nChar);

private:

    void *m_pHCI;
	TBTHCIEventHandler *m_pPacketHandler;
    
	u8 m_RxBuffer[UART_TRANSPORT_BUFFER_SIZE];
	unsigned m_nRxState;
	unsigned m_nRxParamLength;
	unsigned m_nRxInPtr;
    
    u8 m_prefix_byte;

};


#endif
