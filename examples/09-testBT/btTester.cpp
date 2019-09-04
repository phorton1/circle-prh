#include "btTester.h"
#include "kernel.h"
#include <assert.h>
#include <circle/util.h>
#include <circle/sched/scheduler.h>
#include <bt/_sdp_defs.h>

#define log_name "testBT"

#define INQUIRY_SECONDS			12
#define PRE_POPULATE_DEVICES   	1

#define STATE_NONE              	0
#define STATE_SELECT_DEVICE         1
#define STATE_SELECT_UUID           2
#define STATE_STARTING             	99


//-----------------------------------------
// static variable definitions
//-----------------------------------------

struct select_uuid_type
{
	u16 uuid;
	const char *name;
};


struct static_device
{
	const char *addr;
	const char *name;
	u8 use_channel;
};


select_uuid_type select_uuids[] =
{
	{ UUID_PROTO_L2CAP,			 "PROTO_L2CAP" },
	{ UUID_PROTO_SDP,			 "PROTO_SDP" },			
	{ UUID_PROTO_RFCOMM,         "PROTO_RFCOMM" },
	{ UUID_SERVICE_SERIAL_PORT,  "SERVICE_SERIAL_PORT" },
};

// I think the bt05's are BTLE - I never see them in an
// inquiry scan from the (BR/ER) rPi or usb controllers ...

#define lenovo 	"AC:7B:A1:54:13:7E"
#define p10 	"10:D0:7A:84:A2:74"
#define k10		"FE:7D:46:65:82:21"
#define blu		"E4:C8:01:62:95:B3"
#define d24		"7E:2E:46:65:80:31"
#define bt05_1	"00:13:AA:00:0D:7A"
#define bt05_2  "00:13:AA:00:18:5A"

static_device static_devices[] =
{
	{ lenovo, 	"LENOVO-PC2"	 	},		
	{ p10, 		"P10"				},		
	{ k10, 		"K10T"				},
	{ blu, 		"BLU STUDIO X8 HD"	},
	{ d24, 		"D24"				},
	{ bt05_1, 	"prhBT05-01"		},
	{ bt05_2, 	"prhBT05-02"		},
};


#define NUM_UUIDS  			(sizeof(select_uuids)/sizeof(select_uuid_type))
#define NUM_STATIC_DEVICES 	(sizeof(static_devices)/sizeof(static_device))

//-----------------------------------------
// ctor & dtor
//-----------------------------------------

btTester::btTester(
	CDevice *pSerial,
	bluetoothAdapter *pBT) :
	m_pSerial(pSerial),
	m_pBT(pBT),
	m_pSDP(pBT->getSDPLayer()),
	m_pRFCOMM(pBT->getRFCommLayer())
{
	m_rfChannel = 0;
	m_state = STATE_STARTING;
	m_selected_device = 0;
	
	#if PRE_POPULATE_DEVICES
		for (u8 i=0; i<NUM_STATIC_DEVICES; i++)
		{
			pBT->getHCILayer()->addDevice(
				strToBtAddr(static_devices[i].addr),
				static_devices[i].name);
		}
		selectDevice(0);
	#endif
	
	m_pRFCOMM->registerClient(this,rfCallBackStub);
}


btTester::~btTester()
{
}



//------------------------------------------
// utils
//------------------------------------------


void btTester::listDevices()
{
	int dev_num = 0;
	hciLayer *pHCI = m_pBT->getHCILayer();
	hciRemoteDevice *device = pHCI->getDeviceList();
	while (device)
	{
		CString status = "not connected";
		if (device->handle)
			status.Format("connected(0x%04x)",device->handle);
		printf("   %s[%d] %s  %-20s %-18s %s\n",
			device == m_selected_device ? "*" : " ",
			dev_num++,
			addrToString(device->addr),
			device->name,
			(const char *) status,
			device->link_key_type ? "PAIRED" : "");
		device = device->next;
	}
}


void btTester::selectDevice(u8 num)
{
	int dev_num = 0;
	hciLayer *pHCI = m_pBT->getHCILayer();
	hciRemoteDevice *device = pHCI->getDeviceList();
	while (device)
	{
		if (num == dev_num)
		{
			printf("    device %s(%s) selected\n",
				   addrToString(device->addr),
				   device->name);
			m_selected_device = device;
			return;
		}
		dev_num++;
		device = device->next;
	}
	LOG_ERROR("illegal device number: %d",num);
}




//----------------------------------
// rfcomm
//----------------------------------

void btTester::rfCallBackStub(void *pThis, rfChannel *channel, const u8 *buffer, u16 length)
{
	((btTester *) pThis)->rfCallBack(channel,buffer,length);
}


void btTester::rfCallBack(rfChannel *channel, const u8 *buffer, u16 length)
{
	u8 buf[length+1];
	memcpy(buf,buffer,length);
	buf[length-1] = 0;
	
	printf(">RECV %s(0x%04x) channel(%d): %s\n",
		channel->session->lcn->device->name,
		channel->session->lcn->device->handle,
		channel->channel_num,
		buf);
	// display_bytes(0,buffer,length);
}




void btTester::testPrint(const char *format, ...)
	// someday this will work
{
    rfChannel *avail[5];
	int num_avail = m_pRFCOMM->getOpenChannels(avail,5);
	if (!num_avail)
	{
		printf("no open rfcomm channels available for output\n");
		return;
	}
	
	if (num_avail > 1)
		printf("outputting to %d channels ...\n");
	
	for (int i=0; i<num_avail; i++)
	{
		va_list var;
		va_start(var, format);
		CString s;
		s.FormatV(format, var);

		rfChannel *channel = avail[i];
		printf("<SEND %s(0x%04x) channel(%d): %s\n",
			channel->session->lcn->device->name,
			channel->session->lcn->device->handle,
			channel->channel_num,
			(const char *)s);

		m_pRFCOMM->sendData(channel,(const u8 *)(const char *) s, s.GetLength());
	}
}




//-------------------------------------------
// task()
//-------------------------------------------

void btTester::task()
{
	if (m_state == STATE_STARTING)
	{
		m_state = STATE_NONE;
		printf("\n");
		printf("btTester started .... please press:\n");
		printf("    i = to start an inquiry\n");
		printf("    l = to list devices\n");
		printf("    d = to select a device\n");
		printf("    p = to pair with device\n");
		printf("    u = to unpair with a device\n");
		printf("    s = to perform an SDP SEARCH_ATTR request\n");
		
		printf("    r = to connect (open) to a RFCOMM channel\n");
		printf("    o = to output (print) some characters to the RFCOMM channel\n");
		printf("    c = to disconnect (close) the RFCOMM channel\n");
		
		printf("    h = help (to see this list)\n");
		printf("\n");
	}
	
	unsigned int c = 0;
	if (m_pSerial->Read(&c,1))
	{
		c &= 0xff;

		// clear the state machine if !0..9 pressed
		
		if (c < '0' || c > '9')
		{
			m_state = STATE_NONE;
		}

		if (c == 'h')
		{
			m_state = STATE_STARTING;
		}
		
		//-----------------------------
		// inquiry
		//-----------------------------
		
		else if (c == 'i')
		{
			printf("Inquiry is running for %u seconds\n", INQUIRY_SECONDS);
			hciLayer *hci = m_pBT->getHCILayer();
			hci->startInquiry(INQUIRY_SECONDS);
		}

		//------------------------------
		// pairing
		//------------------------------
		
		else if (c == 'u')
		{
			assert(m_selected_device);
			assert(m_selected_device->link_key_type);
			if (m_selected_device &&
				m_selected_device->link_key_type)
			{
				printf("kernel Unpairing from %s(%s) ...\n",
					addrToString(m_selected_device->addr),
					m_selected_device->name);
				m_pBT->getHCILayer()->unpair(m_selected_device);
				
				printf("Unpairing from %s(%s) ...\n",
					addrToString(m_selected_device->addr),
					m_selected_device->name);
				m_selected_device->link_key_type = 0;
				memset(m_selected_device->link_key,0,BT_LINK_KEY_SIZE);
				m_pBT->getHCILayer()->saveDevices();
			}
		}
		
		
		//-----------------------------
		// list & select device & uuid
		//-----------------------------
		
		else if (c == 'l')
		{
			hciLayer *pHCI = m_pBT->getHCILayer();
			printf("\n");
			printf("Listing %d hciRemoteDevices\n",pHCI->getNumDevices());
			listDevices();
		}
		else if (c == 'd')
		{
			printf("\n");
			printf("Select a device:\n");
			listDevices();
			m_state = STATE_SELECT_DEVICE;			
		}
		else if (c == 's')
		{
			printf("\n");
			printf("Select a UUID for SDP request:\n");
			for (u8 i=0; i<NUM_UUIDS; i++)
			{
				printf("    [%d] 0x%04x  %s\n",
					i,
					select_uuids[i].uuid,
					select_uuids[i].name);
			}
			m_state = STATE_SELECT_UUID;			
		}
		else if (c >= '0' && c <= '9')
		{
			if (m_state == STATE_SELECT_DEVICE)
				selectDevice(c - '0');
				
			// DO THE SDP REQUEST
			
			if (m_state == STATE_SELECT_UUID)
			{
				u8 num = c - '0';
				if (num > NUM_UUIDS)
				{
					LOG_ERROR("illegal UUID number %d",num);
				}
				else
				{
					select_uuid_type *pUUID = &select_uuids[num];
					assert(m_selected_device);
					if (m_selected_device)
					{
						printf("calling sdpRequest(%s(%s), 0x%04x(%s), 0x0000, 0xffff) ...\n",
							addrToString(m_selected_device->addr),
							m_selected_device->name,
							pUUID->uuid,
							pUUID->name);
					
						sdpRequest *request = m_pSDP->doSdpRequest(
							m_selected_device->addr,
							pUUID->uuid,
							0x0000, 0xffff);
						
						if (!request)
							LOG_ERROR("Could not call doSdpRequest()!!",0);
					}
				}
			}
		}
		
		
		
		//-----------------------------
		// rfcomm
		//-----------------------------
		
		else if (c == 'c')
		{
			assert(m_rfChannel);
			assert(m_rfChannel->channel_state & RF_CHANNEL_STATE_OPEN);			
			if (m_rfChannel && (m_rfChannel->channel_state & RF_CHANNEL_STATE_OPEN))
			{
				assert(m_rfChannel->session);
				assert(m_rfChannel->session->lcn);
				assert(m_rfChannel->session->lcn->device);
				
				printf("closing m_RFChannel(%s(%s), 0x%02x) ...\n",
					addrToString(m_rfChannel->session->lcn->device->addr),
					m_rfChannel->session->lcn->device->name,
					m_rfChannel->channel_num);
				
				m_pRFCOMM->closeRFChannel(m_rfChannel);
				m_rfChannel = 0;
			}
		}
		else if (c == 'r')
		{
			assert(m_selected_device);
			if (m_selected_device)
			{

				u8 use_channel = 0x03;
				if (!memcmp(m_selected_device->addr,strToBtAddr(lenovo),BT_ADDR_SIZE))
					use_channel = 0x03;		// rfcomm_channel 0x03 for lenovo, from SDP				
				else if (!memcmp(m_selected_device->addr,strToBtAddr(p10),BT_ADDR_SIZE))
					use_channel = 0x02;		// 0x02 for cmManagerBT, service handle 0x00010009
				else
					printf("WARNING: using default channel(0x%02x)\n",use_channel);
			
				printf("calling openRFChannel(%s(%s), 0x%02x) ...\n",
					addrToString(m_selected_device->addr),
					m_selected_device->name,
					use_channel);
			
				m_rfChannel = m_pRFCOMM->openRFChannel(
					m_selected_device->addr,
					use_channel);
			
				if (!m_rfChannel)
					LOG_ERROR("Could not openRFChannel()!!",0);
				else
					printf("opening channel(%d)\n",m_rfChannel->channel_num);
			}
		}
		else if (c == 'o')
		{
			static int count=0;
			LOG("printing 'Hello from rPi Bluetooth (%d)!!'\n",count);
			testPrint("Hello from rPi Bluetooth (%d)!!\n",count++);
		}
		
	}	// m_pSerial->read()

	CScheduler::Get()->Yield();

}	// CKernel::Run()


