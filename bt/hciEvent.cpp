
#include "hciLayer.h"
#include <assert.h>
#include <circle/util.h>
#include <circle/logger.h>


#define PIN_CODE  "1234"


static const char *log_name = "hciEvent";



bool hciLayer::checkDeviceName(hciRemoteDevice *device)
{
	if (!device->name[0])
	{
		hci_remote_name_request_command cmd;
		cmd.header.opcode = HCI_OP_LINK_REMOTE_NAME_REQUEST;
		memcpy(cmd.addr, device->addr, BT_ADDR_SIZE);
		#if HCI_DEVICE_INCLUDE_UNUSED_FIELDS
			cmd.page_scan_repetition_mode = device->page_mode;
		#else
			cmd.page_scan_repetition_mode = 0;
		#endif
		cmd.reserved = 0;
		cmd.clock_offset = HCI_LINK_CLOCK_OFFSET_INVALID;
		LOG_HCI("REMOTE_NAME_REQUEST to %s",addrToString(device->addr));
		sendCommand(&cmd, sizeof cmd);
		return false;
	}
	return true;
}
   

void hciLayer::processEvent(const void *buffer, unsigned length)
{
	assert(m_pClient);
	assert(length >= sizeof(hci_event_header));
	hci_event_header *pHeader = (hci_event_header *) buffer;

	switch (pHeader->event_code)
	{
		case HCI_EVENT_TYPE_COMMAND_STATUS:
		{
			assert(length >= sizeof(hci_command_status_event));
			hci_command_status_event *pCommandstatus = (hci_command_status_event *) pHeader;
			m_can_send_command = pCommandstatus->num_command_packets;
			break;
		}
			
		//---------------------------------------------------------
		// COMMAND_COMPLETE on RESET drives a state machine
		//---------------------------------------------------------
		
		case HCI_EVENT_TYPE_COMMAND_COMPLETE:
		{
			assert(length >= sizeof(hci_command_complete_event));
			hci_command_complete_event *pCommandComplete =(hci_command_complete_event *) pHeader;
			m_can_send_command = pCommandComplete->num_command_packets;

			if (pCommandComplete->status != BT_STATUS_SUCCESS)
			{
				LOG_ERROR("Command 0x%X failed (status 0x%X) %s",
					(unsigned) pCommandComplete->command_opcode,
					(unsigned) pCommandComplete->status,
					getBTErrorString(pCommandComplete->status));
				return;
			}

			// a completed COMMAND_RESET takes two different paths
			// depending on if we are using the onboard BT module,
			// in which case we have to upload the firmware ..
			
			switch (pCommandComplete->command_opcode)
			{
				case HCI_OP_BASEBAND_RESET:
					if (m_pVendor)
						m_pVendor->setup();
					else
						m_HCIBase.setup();
					break;
				
				default:
					if (m_pVendor && !m_pVendor->isSetup())
						m_pVendor->processCommandCompleteEvent(buffer,length);
					else if (!m_HCIBase.isSetup())
						m_HCIBase.processCommandCompleteEvent(buffer,length);
					break;
			}
			break;
			
		}	// HCI_EVENT_TYPE_COMMAND_COMPLETE
		
				
		//----------------------------------------------
		// the bulk of HCI events go here
		//----------------------------------------------
		
		case HCI_EVENT_TYPE_INQUIRY_RESULT:
		{
			LOG_HCI("INQUIRY_RESULT len=%d",length);
			assert(length >= sizeof(hci_inquiry_result_event));
			hci_inquiry_result_event *pEvent = (hci_inquiry_result_event *) pHeader;
			
			for (unsigned i = 0; i < pEvent->num_responses; i++)
			{;
				u8 addr[BT_ADDR_SIZE];
				u8 cls[BT_CLASS_SIZE];
				memcpy(addr, INQUIRY_RESP_BD_ADDR(pEvent, i), BT_ADDR_SIZE);
				memcpy(cls, INQUIRY_RESP_CLASS_OF_DEVICE(pEvent, i), BT_CLASS_SIZE);
				
				hciRemoteDevice *device = findDeviceByAddr(addr);
				LOG_HCI("    inquiry %s %s class(%s) pmode(%d) %s",
					device ? "update" : "new",
					addrToString(addr),
					deviceClassToString(cls),
					INQUIRY_RESP_PAGE_SCAN_REP_MODE(pEvent, i),
					device ? device->name : "(unknown)");
				if (!device)
				{
					device = addDevice(addr,0);
					if (!device) return;
				}
				memcpy(device->addr,addr,BT_ADDR_SIZE);
				memcpy(device->device_class,cls,BT_CLASS_SIZE);
				
				#if HCI_DEVICE_INCLUDE_UNUSED_FIELDS
					device->page_rep_mode = INQUIRY_RESP_PAGE_SCAN_REP_MODE(pEvent, i);					
				#endif
				
				if (!checkDeviceName(device))
					m_num_name_requests++;
					
				m_pClient->receiveEvent(HCC_INQUIRY_DEVICE_FOUND,device);
			}

			break;
		} 

		case HCI_EVENT_TYPE_INQUIRY_COMPLETE:
		{
			assert(length >= sizeof(hci_inquiry_complete_event));
			hci_inquiry_complete_event *pEvent = (hci_inquiry_complete_event *) pHeader;
			LOG("INQUIRY_COMPLETE %s %s",
				pEvent->status == BT_STATUS_SUCCESS ? "OK" : "ERROR",
				pEvent->status == BT_STATUS_SUCCESS ? "" :
				getBTErrorString(pEvent->status));
			if (!m_num_name_requests)
				m_pClient->receiveEvent(HCC_INQUIRY_COMPLETE,0);
			break;
		}

		case HCI_EVENT_TYPE_REMOTE_NAME_REQUEST_COMPLETE:
		{
			hci_remote_name_complete_event *pEvent = (hci_remote_name_complete_event *) pHeader;		
			
			LOG_HCI("NAME_REQUEST_COMPLETE %s %s %s",
				addrToString(pEvent->addr),
				pEvent->status == BT_STATUS_SUCCESS ? "OK" : "ERROR",
				pEvent->status == BT_STATUS_SUCCESS ? (char *) pEvent->remote_name : getBTErrorString(pEvent->status));
				
			if (pEvent->status == BT_STATUS_SUCCESS)
			{
				hciRemoteDevice *device = findDeviceByAddr(pEvent->addr);
				if (!device)
				{
					LOG_ERROR("unknown device in name complete event!",0);
					return;
				}
				memcpy(device->name,pEvent->remote_name,BT_NAME_SIZE);
				m_pClient->receiveEvent(HCC_INQUIRY_NAME_FOUND,device);
			}
			
			if (m_num_name_requests)
			{
				m_num_name_requests--;
				if (!m_num_name_requests)
					m_pClient->receiveEvent(HCC_INQUIRY_COMPLETE,0);
			}
			
			break;			
		}
		
	
		//-----------------------
		// prh my additions
		//-----------------------

		case HCI_EVENT_TYPE_CONNECTION_REQUEST:
		{
			hci_connection_request_event *pEvent = (hci_connection_request_event *) pHeader;		
			LOG_HCI("CONNECTION_REQUEST %s packet_type(0x%04x) pr_mode(0x%02x) p_mode(0x%02x) clock(0x%04x) switch(0x%04x)",
				addrToString(pEvent->addr),
				pEvent->packet_type,
				pEvent->page_rep_mode,
				pEvent->page_mode,
				pEvent->clock_offset,
				pEvent->switch_flag);

			hciRemoteDevice *device = findDeviceByAddr(pEvent->addr);
			if (!device)
			{
				device = addDevice(pEvent->addr,0);
				if (!device) return;
			}

			#if HCI_DEVICE_INCLUDE_UNUSED_FIELDS
				device->packet_type     = pEvent->packet_type;
				device->page_rep_mode   = pEvent->page_rep_mode;
				device->page_mode		= pEvent->page_mode;
				device->clock_offset    = pEvent->clock_offset;
			#endif
			
			// if the device already has an open handle,
			// we will obviate it and start a new one here
			
			if (device->handle)
			{
				LOG_HCI("Warning: handle(0x%04x) for %s already existed",
					device->handle,
					addrToString(device->addr));
				m_pClient->receiveEvent(HCC_EVENT_DISCONNECTED,device);
				device->handle = 0;
			}
			
			// assign a new handle
			
			device->handle = m_next_hci_handle++;
			checkDeviceName(device);
			m_pClient->receiveEvent(HCC_EVENT_CONNECTED,device);
			
			// send a connection accepted command

			LOG_HCI("sending HCI ACCEPT_CONNECTION_REQUEST command",0);
			hci_accept_connection_command cmd;
			cmd.header.opcode = HCI_OP_LINK_ACCEPT_CONNECTION_REQUEST;
			memcpy(cmd.addr,pEvent->addr,BT_ADDR_SIZE);
			cmd.role = 1;
			sendCommand(&cmd, sizeof(cmd));
			break;
		}
		
		case HCI_EVENT_TYPE_CONNECTION_COMPLETE:
		{
			hci_connection_complete_event *pEvent = (hci_connection_complete_event *) pHeader;		
			LOG_HCI("CONNECTION_COMPLETE %s %s %s  handle=0x%04x",
				addrToString(pEvent->addr),
				pEvent->status == BT_STATUS_SUCCESS ? "OK" : "ERROR",
				pEvent->status == BT_STATUS_SUCCESS ? "" : getBTErrorString(pEvent->status),
				pEvent->handle);
			
			hciRemoteDevice *device = findDeviceByAddr(pEvent->addr);
			if (pEvent->status == BT_STATUS_SUCCESS)
			{
				if (!device)
				{
					device = addDevice(pEvent->addr,0);
					if (!device) return;
				}
				device->handle 	  = pEvent->handle;
				#if HCI_DEVICE_INCLUDE_UNUSED_FIELDS
					device->link_type = pEvent->link_type;
					device->encrypt   = pEvent->encrypt;
				#endif
				m_pClient->receiveEvent(HCC_EVENT_CONNECTED,device);
			}
			else
			{
				m_pClient->receiveEvent(HCC_EVENT_CONNECTION_ERROR,device);
				if (device)
					device->handle = 0;
			}
			checkDeviceName(device);
			break;
		}

		case HCI_EVENT_TYPE_DISCONNECTION_COMPLETE:
		{
			hci_disconnect_complete_event *pEvent = (hci_disconnect_complete_event *) pHeader;		
			LOG_HCI("DISCONNECTION_COMPLETE handle(0x%04x) %s %s %s",
				pEvent->handle,
				pEvent->status ? "ERROR: ": "OK",
				pEvent->status ? getBTErrorString(pEvent->status) : "",
				pEvent->reason ? getBTErrorString(pEvent->reason) : "");
			
			hciRemoteDevice *device = findDeviceByHandle(pEvent->handle);
			if (device)
			{
				m_pClient->receiveEvent(HCC_EVENT_DISCONNECTED,device);
				device->handle = 0;
			}
			break;
		}
		
		case HCI_EVENT_TYPE_PIN_CODE_REQUEST:
		{
			hci_pin_code_request_event *pEvent = (hci_pin_code_request_event *) pHeader;
			LOG_HCI("PIN_CODE_REQUEST from %s SENDING '%s'",
				addrToString(pEvent->addr),PIN_CODE);

			hci_pin_code_reply_command cmd;
			cmd.header.opcode = HCI_OP_LINK_PIN_CODE_REQUEST_REPLY;
			cmd.pin_code_len = strlen(PIN_CODE);
			memset(cmd.pin_code,0,BT_PIN_CODE_SIZE);
			memcpy(cmd.addr,pEvent->addr,BT_ADDR_SIZE);
			memcpy(cmd.pin_code,PIN_CODE,strlen(PIN_CODE));
			sendCommand(&cmd, sizeof(cmd));
			break;
		}

		case HCI_EVENT_TYPE_AUTHENTICATION_COMPLETE:
		{
			hci_authentication_complete_event *pEvent = (hci_authentication_complete_event *) pHeader;
			LOG_HCI("AUTHENTICATION_COMPLETE handle(0x%04x) %s%s",
				pEvent->handle,
				pEvent->status ? "ERROR: ": "OK",
				pEvent->status ? getBTErrorString(pEvent->status) : "");
			hciRemoteDevice *device = findDeviceByHandle(pEvent->handle);
			assert(device);
			break;
		}
		
		case HCI_EVENT_TYPE_LINK_KEY_NOTIFICATION:
		{
			hci_link_key_notification_event *pEvent = (hci_link_key_notification_event *) pHeader;
			LOG_HCI("LINK_KEY_NOTIFICATION %s key_type(0x%02x)",
				addrToString(pEvent->addr),
				((hci_link_key_notification_event *) pHeader)->key_type);
			hciRemoteDevice *device = findDeviceByAddr(pEvent->addr);
			assert(device);
			memcpy(device->link_key,pEvent->link_key,BT_LINK_KEY_SIZE);
			
			// unfortunately the "combined" type is 0x00
			// and I want to use the link_key_type to indicate pairing.
			// so I set it to the otherwise unused value of 0x02 if it zero
			
			if (!pEvent->key_type)
				pEvent->key_type = 0x02;
			device->link_key_type = pEvent->key_type;
			
			#if HCI_USE_FAT_DATA_FILE
				saveDevices();
			#endif
			break;
		}
		case HCI_EVENT_TYPE_LINK_KEY_REQUEST :
		{
			// if there is a link_key, as evidenced by device->link_key_type,
			// send a link_key_response, otherwise, reject the request and the
			// remote device will prompt for a pin
			
			hci_link_key_notification_event *pEvent = (hci_link_key_notification_event *) pHeader;
			LOG_HCI("LINK_KEY_REQUEST from %s",addrToString(pEvent->addr));
			hciRemoteDevice *device = findDeviceByAddr(pEvent->addr);
			assert(device);
			if (device && device->link_key_type)
			{
				hci_link_key_request_reply_command cmd;
				cmd.header.opcode = HCI_OP_LINK_LINK_KEY_REQUEST_REPLY;
				memcpy(cmd.addr, device->addr, BT_ADDR_SIZE);
				memcpy(cmd.link_key, device->link_key, BT_LINK_KEY_SIZE);
				LOG_HCI("sending LINK_KEY_REPLY_COMMAND",0);
				sendCommand(&cmd, sizeof cmd);
			}
			else
			{
				hci_link_key_request_reject_command cmd;
				cmd.header.opcode = HCI_OP_LINK_LINK_KEY_REQUEST_NEGATIVE_REPLY;
				memcpy(cmd.addr, pEvent->addr, BT_ADDR_SIZE);
				LOG_HCI("sending LINK_KEY_REJECT_COMMAND",0);
				sendCommand(&cmd, sizeof cmd);
			}
			break;
		}
			
		
		case HCI_EVENT_TYPE_PAGE_SCAN_REPETITION_MODE_CHANGE:
		{
			hci_page_scan_rep_mode_change_event *pEvent = (hci_page_scan_rep_mode_change_event *) pHeader;
			LOG_HCI("PAGE_SCAN_REPETITION_MODE_CHANGE %s mode=0x%02x",
				addrToString(pEvent->addr),
				pEvent->page_rep_mode);
			#if HCI_DEVICE_INCLUDE_UNUSED_FIELDS
				hciRemoteDevice *device = findDeviceByAddr(pEvent->addr);
				assert(device);
				if (device)
					device->page_rep_mode = pEvent->page_rep_mode;
			#endif
			break;
		}
		case HCI_EVENT_TYPE_MAX_SLOTS_CHANGE:
		{
			LOG_HCI("MAX_SLOTS_CHANGE handle=0x%04x  slots=0x%02x",
				((hci_max_slots_change_event *) pHeader)->handle,
				((hci_max_slots_change_event *) pHeader)->max_slots);
			#if HCI_DEVICE_INCLUDE_UNUSED_FIELDS
				hciRemoteDevice *device = findDeviceByHandle(
					((hci_max_slots_change_event *) pHeader)->handle);
				assert(device);
				device->max_slots = ((hci_max_slots_change_event *) pHeader)->max_slots;
			#endif
			break;
		}
		case HCI_EVENT_LINK_SUPER_TIMEOUT_CHANGED_EVT :
		{
			hci_link_timeout_changed_event *pEvent = (hci_link_timeout_changed_event *) pHeader;
			LOG_HCI("SUPER_TIMEOUT_CHANGED_EVT handle=0x%04x timeout=0x%04x",
				pEvent->handle,
				pEvent->timeout);
			#if HCI_DEVICE_INCLUDE_UNUSED_FIELDS
				hciRemoteDevice *device = findDeviceByHandle(pEvent->handle);
				assert(device);
				device->timeout = pEvent->timeout;
			#endif
			break;
		}
		case HCI_EVENT_TYPE_NUMBER_OF_COMPLETED_PACKETS:
		{
			// TODO: should implement this to increment per-handle counter
			// for sends ?!?!!
			
			hci_number_of_completed_packets_event *pEvent = (hci_number_of_completed_packets_event *) pHeader;
			assert(pEvent->num_entries);
			LOG_HCI("NUMBER_OF_COMPLETED_PACKETS num(%d) handle[0]=0x%04x completed[0]=%d",
				pEvent->num_entries,
				pEvent->data[0],
				pEvent->data[pEvent->num_entries]);
			m_can_send_data += pEvent->data[pEvent->num_entries];

			if (pEvent->num_entries > 1)
			{
				for (u8 i=1; i<pEvent->num_entries; i++)
				{
					LOG_HCI("handle[%d]=0x%04x completed[$]=%d",
						i,pEvent->data[i],
						i,pEvent->data[i+pEvent->num_entries]);
					m_can_send_data+= pEvent->data[i+pEvent->num_entries];
				}
			}
			break;
		}
		
		default:
			LOG("unhandled 0x%02x %s len=%d",
				pHeader->event_code,
				getBTEventName(pHeader->event_code),
				pHeader->length);
			display_bytes("   event:",(u8 *) buffer,length);
			break;
			
	}	//  switch(event_code)
}	// 	hciLayer::processEvent()



