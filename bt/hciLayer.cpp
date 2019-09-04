//
// hciLayer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "hciLayer.h"
#include <assert.h>
#include <circle/util.h>
#include <circle/alloc.h>
#include <circle/logger.h>
// for debugging only
#include "_hci_vendor_defs.h"


static const char *log_name = "hci";

// The HCI Layer determines the name of the device,
// based on whether we use the UART or USB Transport

#define USB_BT_DEVICE_NAME "Raspberry Pi USB"
#define UART_BT_DEVICE_NAME "Raspberry Pi UART"

#define WARN_PENDING_SENDS   1
	// LOG a warning if process() skips sending
	// an hci command or data packet.
	
#define SHOW_OUT_DATA_ENQUEUED  		0
#define SHOW_OUT_COMMAND_ENQUEUED  		0
#define SHOW_OUT_DATA_DEQUEUED  		0
#define SHOW_OUT_COMMAND_DEQUEUED  		0
#define SHOW_IN_DATA_DEQUEUED  			0
#define SHOW_IN_EVENT_DEQUEUED    		0
	// show hci packets enqueued or dequeued
	// these happen in process, outside of the irq


#define LCAP_STARTING_TXN_ID   17
#define LCAP_STARTING_CID      0x70
	// different numbers help in debugging raw bytes



//-------------------------------------
// ctor
//-------------------------------------


hciLayer::hciLayer() :
	m_HCIBase(this,BT_USE_DEVICE_CLASS),
	m_pTransport(0),
	m_pVendor(0)
{
    m_pFileSystem = 0;
	m_pBuffer = 0;	
	init();
}


hciLayer::~hciLayer(void)
{
	while (m_first_device)
	{
		hciRemoteDevice *next = m_first_device->next;
		free(m_first_device);
		m_first_device = next;
	}

	init();
	if (m_pBuffer)
		delete m_pBuffer;
	if (m_pVendor)
		delete m_pVendor;
	m_pBuffer = 0;
	m_pVendor = 0;
	m_pTransport = 0;
	m_pFileSystem = 0;
}


void hciLayer::init()
{
	m_num_devices = 0;
	m_first_device = 0;
	m_last_device = 0;
	
	m_length = 0;
	m_offset = 0;
	m_packet_prefix = 0;

	m_can_send_data = 5;
	    // prh should be 1, works better at 5 for usb transport
	    // 5 *may* break order of operations on uart transport
	m_can_send_command = 1;
	
	m_next_hci_handle = 0x0b;
	m_num_name_requests = 0;
}



//----------------------------------
// device primitives
//----------------------------------

void hciLayer::removeDevice(hciRemoteDevice *device)
{
	LOG_HCI("removing device",addrToString(device->addr));
	hciRemoteDevice *prev = device->prev;
	hciRemoteDevice *next = device->next;
	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;
	if (device == m_first_device)
		m_first_device = next;
	if (device == m_last_device)
		m_first_device = prev;
	m_num_devices--;
	free(device);
}



hciRemoteDevice *hciLayer::addDevice(const u8 *addr, const char *name)
{
	LOG_HCI("addDevice(%s) %s",addrToString(addr),name ? name : "");
	hciRemoteDevice *device = findDeviceByAddr(addr);
	if (device)
	{
		LOG_ERROR("device %s already exists",addrToString(addr));
		return device;
	}
	
	device = (hciRemoteDevice *) malloc(sizeof(hciRemoteDevice));
	assert(device);
	if (!device) return 0;
	
	m_num_devices++;
	memset(device,0,sizeof(hciRemoteDevice));
		
	device->prev = m_last_device;
	if (m_last_device)
		m_last_device->next = device;
	m_last_device = device;
	
	if (!m_first_device)
		m_first_device = device;
		
	memcpy(device->addr,addr,BT_ADDR_SIZE);
	device->next_lcap_id = LCAP_STARTING_TXN_ID;
	device->next_lcap_cid = LCAP_STARTING_CID;
	if (name)
		strcpy(device->name,name);

	return device;
}




hciRemoteDevice *hciLayer::findDeviceByAddr(const u8 *addr)
{
	hciRemoteDevice *device = m_first_device;
	while (device)
	{
		if (!memcmp(addr,device->addr,BT_ADDR_SIZE))
			return device;
		device = device->next;
	}
	return 0;
}


hciRemoteDevice *hciLayer::findDeviceByName(const char *name)
{
	hciRemoteDevice *device = m_first_device;
	while (device)
	{
		if (!strcmp(name,device->name))
			return device;
		device = device->next;
	}
	return 0;
}


hciRemoteDevice *hciLayer::findDeviceByHandle(u16 handle)
{
	assert(handle);
	hciRemoteDevice *device = m_first_device;
	while (device)
	{
		if (handle == (device->handle  & 0x0fff))
			return device;
		device = device->next;
	}
	return 0;
}

void hciLayer::unpair(hciRemoteDevice *device)
{
	assert(device);
	assert(device->link_key_type);
	if (device &&
		device->link_key_type)
	{
		LOG("unpair %s(%s) ...\n",
			addrToString(device->addr),
			device->name);
		device->link_key_type = 0;
		memset(device->link_key,0,BT_LINK_KEY_SIZE);
		#if HCI_USE_FAT_DATA_FILE
			saveDevices();
		#endif
	}
}




#if HCI_USE_FAT_DATA_FILE

	#define DEBUG_LOAD   0
	#define HCI_FILENAME "SD:hci_devices.txt"

	char *linkKeyToString(u8 *link_key)
	{
		static char buf[2 * BT_LINK_KEY_SIZE + 1];
		char *p = buf;
		for (u8 i=0; i<BT_LINK_KEY_SIZE; i++)
		{
			u8 n1 = link_key[i];
			u8 n0 = n1 >> 4;
			n1 &= 0xf;
			*p++ = (n0 > 9) ? 'A' + n0-10 : '0' + n0;
			*p++ = (n1 > 9) ? 'A' + n1-10 : '0' + n1;
		}
		return buf;
	}
	
	
	void hciLayer::loadDevices()
	{
		FIL file;
		if (f_open(&file, HCI_FILENAME, FA_READ | FA_OPEN_EXISTING) == FR_OK)
		{
			LOG("loadDevices()",0);
			#define MAX_LINE   	255
			char buf[MAX_LINE];
			char *line = f_gets(buf,MAX_LINE,&file);
			while (line)
			{
				#if DEBUG_LOAD
					printf("line=%s",line);
				#endif
				
				char *addr_str = line;
				char *p = &line[3*BT_ADDR_SIZE + 1];
				const char *name = (const char *) p;
				
				
				while (*p != '"') p++;	// better be one!
				*p++ = 0;				// set the closing quote to a zero

				#if DEBUG_LOAD
					display_bytes("    addr=",strToBtAddr(addr_str),BT_ADDR_SIZE);
					printf("    name=%s\n",name);
				#endif
				
				hciRemoteDevice *device = addDevice(
					strToBtAddr(addr_str),name);
				assert(device);
				
				p++;					// skip the comma
				u8 n1 = *p++;
				u8 n2 = *p++;
				device->link_key_type =
					(n2 >= 'A' ? 10+n2-'A' : n2-'0') +
					((n1 >= 'A' ? 10+n1-'A' : n1-'0') << 4);

				p++;					// skip the comma
				for (u8 i=0; i<BT_LINK_KEY_SIZE; i++)
				{
					u8 n1 = *p++;
					u8 n2 = *p++;
					device->link_key[i] = 
						(n2 >= 'A' ? 10+n2-'A' : n2-'0') +
						((n1 >= 'A' ? 10+n1-'A' : n1-'0') << 4);
				}
				
				#if DEBUG_LOAD
					printf("    link_key_type=0x%02x\n",device->link_key_type);
					display_bytes("    link_key",device->link_key,BT_LINK_KEY_SIZE);
				#endif
				line = f_gets(buf,MAX_LINE,&file);
			}
			f_close(&file);		
		}
		else
		{
			LOG("warning: could not open %s",HCI_FILENAME);
		}
	}
	
	void hciLayer::saveDevices()
	{
		LOG("saveDevices()",0);
		
		FIL file;
		if (f_open(&file, HCI_FILENAME, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
		{
			hciRemoteDevice *device = m_first_device;
			while (device)
			{
				if (device->link_key_type)
				{
					int ok = f_printf(&file,"%s,\"%s\",%02x,%s\n",
						addrToString(device->addr),
						device->name,
						device->link_key_type,
						linkKeyToString(device->link_key));
					if (!ok)
					{
						LOG_ERROR("Could not write device to %s ...",HCI_FILENAME);
					}
				}
				device = device->next;
			}
			f_close(&file);		
		}
		else
		{
			LOG_ERROR("Could not open %s for writing!",HCI_FILENAME);
		}
	}
#endif


//----------------------------------------
// initialize
//----------------------------------------

bool hciLayer::initialize(btTransportBase *pTransport
    #if HCI_USE_FAT_DATA_FILE
        ,FATFS *pFileSystem
    #endif
	)
{
	m_pTransport = pTransport;

	// LOG("hciLayer::initialize()",0);
	
	m_pTransport->registerPacketHandler(this,receiveTransportStub);
	if (m_pTransport->isUartTransport())
	{
		m_HCIBase.setLocalName(UART_BT_DEVICE_NAME);
		m_pVendor = new hciVendor(&m_HCIBase);
		assert(m_pVendor);
	}
	else
	{
		m_HCIBase.setLocalName(USB_BT_DEVICE_NAME);
	}
	
	// load the persistent device data
	
    #if HCI_USE_FAT_DATA_FILE
        m_pFileSystem = pFileSystem;
		loadDevices();
    #endif

	// start the state machine(s) by resetting the HCI host
	
	m_HCIBase.reset();
	return true;
}


//---------------------------------------
// command and event processing
//---------------------------------------


void hciLayer::sendCommand(const void *buffer, unsigned length)
{
	assert(length <= MAX_SIZE_BYTE);
	hci_command_header *hdr = (hci_command_header *) buffer;
	hdr->length = length - sizeof(hci_command_header);
	
	#if SHOW_OUT_COMMAND_ENQUEUED
		// don't show HCI_OP_VENDOR_WRITE_RAM
		if (hdr->opcode != HCI_OP_VENDOR_WRITE_RAM)
			display_bytes("<hci eqoc",(u8 *)buffer,length);
	#endif
					
	m_command_queue.enqueue(buffer, length);
}

void hciLayer::sendData(const void *buffer, unsigned length)
{
	hci_data_header *hdr = (hci_data_header *) buffer;
	hdr->len = length - sizeof(hci_data_header);

	#if SHOW_OUT_DATA_ENQUEUED
		display_bytes("<hci eqod",(u8 *)buffer,length);
	#endif

	m_send_data_queue.enqueue(buffer, length);
}




void hciLayer::process(void)
{
	btBuffer *pEntry;
	assert(m_pClient);
	assert(m_pTransport);
	
	// process incoming events and data first
	
	while ((pEntry = m_event_queue.dequeue()))
	{
		#if SHOW_IN_EVENT_DEQUEUED
			// don't show HCI_OP_VENDOR_WRITE_RAM
			hci_event_header *hdr = (hci_event_header *) pEntry->buffer;
			hci_command_complete_event *cmd = (hci_command_complete_event *) pEntry->buffer;
			if (hdr->event_code != HCI_EVENT_TYPE_COMMAND_COMPLETE ||
				cmd->command_opcode != HCI_OP_VENDOR_WRITE_RAM)
				display_bytes(">hci dqievt",pEntry->buffer,pEntry->length);
		#endif
		processEvent(pEntry->buffer,pEntry->length);
		delete pEntry;
	}
	
	while ((pEntry = m_recv_data_queue.dequeue()))
	{
		#if SHOW_IN_DATA_DEQUEUED
			display_bytes(">hci dqid",pEntry->buffer,pEntry->length);
		#endif
		m_pClient->receiveData(pEntry->buffer,pEntry->length);
		delete pEntry;
	}
	
	// process outgoing commands

	#if WARN_PENDING_SENDS
		if (m_command_queue.avail())
		{
			if (!m_can_send_command)
			{
				LOG("warning: waiting to send HCI command",9);
			}
			else
			{
	#endif
	
			while (m_can_send_command &&
			   (pEntry = m_command_queue.dequeue()))
			{
				#if SHOW_OUT_COMMAND_DEQUEUED
					// don't show HCI_OP_VENDOR_WRITE_RAM
					if (((hci_command_header *)pEntry->buffer)->opcode != HCI_OP_VENDOR_WRITE_RAM)
						display_bytes("<hci dqoc",pEntry->buffer,pEntry->length);
				#endif
				
				if (!m_pTransport->SendHCICommand(pEntry->buffer, pEntry->length))
				{
					delete pEntry;
					LOG_ERROR("Could not send HCI command",0);
					break;
				}
				delete pEntry;
				m_can_send_command--;
			}
			
	#if WARN_PENDING_SENDS
			}
		}
	#endif
	
	
	// process outgoing data
	
	#if WARN_PENDING_SENDS
		if (m_send_data_queue.avail())
		{
			static u32 wait_count = 0;
			static u32 wait_time = 0;
		
			if (!m_can_send_data)
			{
				wait_count++;
				u32 now = CTimer::Get()->GetTicks();
				if (now > wait_time + 1000)
				{
					wait_time = now;
					LOG("warning: waiting to send HCI data %d",wait_count);
				}
			}
			else
			{
	#endif
	
			while (m_can_send_data &&
				   (pEntry = m_send_data_queue.dequeue()))
			{
				// static u32 sent_count = 0;
				// printf("    sending data packet #%d\n",++sent_count);
				assert(m_pTransport);
				#if WARN_PENDING_SENDS
					wait_count = 0;
				#endif
				
				#if SHOW_OUT_DATA_DEQUEUED
					display_bytes("<hci dqod",pEntry->buffer,pEntry->length);
				#endif
				
				if (!m_pTransport->SendHCIData(pEntry->buffer, pEntry->length))
				{
					delete pEntry;
					LOG_ERROR("Could not send HCI data",0);
					break;
				}
				delete pEntry;
				m_can_send_data--;
			}
			
	#if WARN_PENDING_SENDS
			}
		}
	#endif
	
	
}	// HCILayer::Process()



//--------------------------------------------------------
// Accept Data from the Transport
//--------------------------------------------------------


void hciLayer::receiveTransportStub(void *pHCILayer, u8 hci_prefix, const void *buffer, unsigned length)
{
	hciLayer *pThis = (hciLayer *) pHCILayer;
	assert(pThis != 0);
	pThis->receiveTransport(hci_prefix, buffer, length);
}



void hciLayer::receiveTransport(u8 hci_prefix, const void *const_buffer, unsigned length)
	// The HCI prefix has already been stripped out
{
	// if starting a new packet,
	// get the HCI packet prefix byte (packet type)
	// and determine m_length for possible subsequent calls
	
	u8 *buffer = (u8 *) const_buffer;
	
	assert(buffer != 0);
	assert(length > 0);

	if (m_offset == 0)
	{
		m_packet_prefix = hci_prefix;
		if (m_packet_prefix == HCI_PREFIX_DATA)
		{
			if (length < 4)
			{
				LOG("Short ACL packet ignored",0);
				return;
			}
			m_length = (*(u16 *) &buffer[2]) + 4;
				// after word hci_handle is word of length
				// add those four bytes back into the total packet length
			// assert(m_length <= MAX_SIZE_BYTE);
				// right now we don't support packets with
				// more than than 255 bytes of data
		}
		else if (length < sizeof(hci_event_header))
		{
			LOG("Short Event Packet ignored",0);
			return;
		}
		else
		{
			m_length = buffer[1] + 2;
				// after the event type byte the length is a byte,
				// add them back in to get the total packet length
		}
		assert(m_pBuffer == 0);
		m_pBuffer = new btBuffer(m_length);
	}
	else
	{
		assert(hci_prefix == m_packet_prefix);
	}
	
	// copy to our buffer and return for more
	// if we have not reached the length ..
	
	memcpy(m_pBuffer->buffer + m_offset, buffer,length);
	m_offset += length;
	if (m_offset < m_length)
		return;

	// enqueue the packet to either the incoming
	// event, or data, queue
	
	if (m_packet_prefix == HCI_PREFIX_DATA)
		m_recv_data_queue.enqueue(m_pBuffer);
	else
		m_event_queue.enqueue(m_pBuffer);

	m_pBuffer = 0;	
	m_length = 0;
	m_offset = 0;
}



//----------------------------------------------------
// high level API methods (hci commands)
//----------------------------------------------------

void  hciLayer::closeConnection(hciRemoteDevice *device)
{
	LOG("closeConnection(%s) handle=0x%04x",addrToString(device->addr),device->handle);
	hci_disconnection_request cmd;
	cmd.header.opcode = HCI_OP_LINK_DISCONNECT;
	cmd.disconnect_handle = device->handle;
	cmd.reason        = 0x16;		// 0x16 == Connection Terminated by Local Host
	sendCommand(&cmd, sizeof(cmd));
}


hciRemoteDevice * hciLayer::startConnection(u8 *addr)
{
	LOG("hciLayer::startConnection(%s)",addrToString(addr));
	
	// see if there's an existing device
	
	hciRemoteDevice *device = findDeviceByAddr(addr);
	if (device && device->handle)
	{
		assert(m_pClient);

		if (device->handle & HCI_HANDLE_CONNECTING)
		{
			LOG("WARNING - device already has a pending hci connection",
				addrToString(device->addr));
			return device;
		}
		if (device->handle & HCI_HANDLE_ERROR)
		{
			LOG("WARNING - restarting connection to errored device %s",
				addrToString(device->addr));
			// fall thru
		}
		else
		{
			LOG("WARNING - device %s already has hci_handle=0x%04x",
				addrToString(device->addr),
				device->handle);
			m_pClient->receiveEvent(HCC_EVENT_CONNECTED,device);
			return device;
		}
	}
	
	// add a new device if needed
	
	if (!device)
		device = addDevice(addr,0);
	device->handle = HCI_HANDLE_CONNECTING;
	
	// send the connnection request
	
	hci_create_connection_command cmd;
	cmd.header.opcode = HCI_OP_LINK_CREATE_CONNECTION;
	memcpy(cmd.addr,addr,BT_ADDR_SIZE);
	cmd.packet_type   = 0x0008;
	cmd.page_rep_mode = 0x01;
	cmd.page_mode     = 0x00;
	cmd.clock_offset  = 0x0000;
	cmd.switch_roles  = 0x00;
	sendCommand(&cmd, sizeof(cmd));
	
	return device;
}



void hciLayer::cancelInquiry()
{
	LOG("cancelInquiry()",0);
	hci_command_header cmd;
	cmd.opcode = HCI_OP_LINK_INQUIRY_CANCEL;
	cmd.length = 0;
	sendCommand(&cmd, sizeof cmd);
}


void hciLayer::startInquiry(unsigned nSeconds)
{
	LOG("startInquiry(%d)",nSeconds);
	assert(1 <= nSeconds && nSeconds <= 61);

	hci_inquiry_command cmd;
	cmd.header.opcode = HCI_OP_LINK_INQUIRY;
	cmd.lap[0] = HCI_LINK_INQUIRY_LAP_GIAC       & 0xFF;
	cmd.lap[1] = HCI_LINK_INQUIRY_LAP_GIAC >> 8  & 0xFF;
	cmd.lap[2] = HCI_LINK_INQUIRY_LAP_GIAC >> 16 & 0xFF;
	cmd.inquiry_length = HCI_LINK_INQUIRY_LENGTH(nSeconds);
	cmd.num_responses = HCI_LINK_INQUIRY_NUM_RESPONSES_UNLIMITED;
	sendCommand(&cmd, sizeof cmd);
}

