#ifndef _btTester_h_
#define _btTester_h_

#include <assert.h>
#include <circle/device.h>
#include <bt/bluetooth.h>


struct select_uuid_type;


class btTester
{
public:

	btTester(CDevice *pSerial, bluetoothAdapter *pBT);
	~btTester();

	void task();
	
private:
	
	CDevice			 	*m_pSerial;
	bluetoothAdapter 	*m_pBT;
	sdpLayer 		 	*m_pSDP;
	rfcommLayer 	 	*m_pRFCOMM;
	rfChannel 		 	*m_rfChannel;

	u16  				m_state;
	hciRemoteDevice  	*m_selected_device;
	
	void listDevices();
	void selectDevice(u8 num);

	void testPrint(const char *format, ...);
	void rfCallBack(rfChannel *channel, const u8 *buffer, u16 length);
	static void rfCallBackStub(void *pThis, rfChannel *channel, const u8 *buffer, u16 length);

	
};

#endif
