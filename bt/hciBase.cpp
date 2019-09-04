//
// hciBase.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2017  R. Stange <rsta2@o2online.de>
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

#include "_hci_defs.h"
#include "hciBase.h"
#include "hciLayer.h"
#include <assert.h>
#include <circle/util.h>
#include <circle/logger.h>

#define WRITE_EIR  					0
	// on UART transport, this will write an extended inquiry
	// response to the hardware. Not used on (my) usb dongle.
	
#define DEBUG_ADDL_CONTROLLER_INFO  0


#if DEBUG_ADDL_CONTROLLER_INFO
#include "hciDebug.cpp"
#endif


static const char *log_name = "hciBase";


hciBase::hciBase(hciLayer *pHCILayer, u32 device_class)
:
	m_pHCI(pHCILayer),
	m_is_setup(false),
	m_device_class(device_class)
{
}



hciBase::~hciBase(void) 
{
}


void hciBase::setLocalName(const char *local_name)
{
	memset(m_local_name, 0, sizeof m_local_name);
	strncpy((char *) m_local_name, local_name, strlen(local_name));
}


void hciBase::reset()
	// called from hciLayer::initialize()
{
	LOG("reset()",0);
	hci_command_header cmd;
	cmd .opcode = HCI_OP_BASEBAND_RESET;
	m_pHCI->sendCommand(&cmd, sizeof cmd);
	m_is_setup = false;
}


void hciBase::setup()
	// called from hciLayer::Intialize() in response
	// to COMMAND_RESET_COMPLETE, or by hciVendor after
	// it has loaded the firmware in response to
	// HCI_OP_VENDOR_LAUNCH_RAM.  We send out the
	// read BD_ADDR command, which starts the base
	// state machine ... 
{
	LOG("setup()",0);
	hci_command_header cmd;
	cmd.opcode = HCI_OP_INFORM_READ_BD_ADDR;
	m_pHCI->sendCommand(&cmd, sizeof cmd);
	m_is_setup = false;
}



//-------------------------------------------
// static externs provided in _hci_defs.h
//-------------------------------------------



const char *addrToString(const u8 *addr)
{
    CString *s = new CString();
    s->Format("%02X:%02X:%02X:%02X:%02X:%02X",
        (unsigned)addr[5],
        (unsigned)addr[4],
        (unsigned)addr[3],
        (unsigned)addr[2],
        (unsigned)addr[1],
        (unsigned)addr[0]);
    static char buf[20];
    strcpy(buf,(const char *)*s);
	delete s;
    return buf;
}

const char *deviceClassToString(const u8 *cls)
{
    CString *s = new CString();
    s->Format("%02X%02X%02X",
        (unsigned)cls[0],
        (unsigned)cls[1],
        (unsigned)cls[2]);
    static char buf[20];
    strcpy(buf,(const char *)*s);
	delete s;
    return buf;
}


const u8 *strToBtAddr(const char *str)
{
	static u8 buf[BT_ADDR_SIZE];
	u8 *op = &buf[BT_ADDR_SIZE-1];
	const char *ip = str;
	
	int len = 0;
	while (len < BT_ADDR_SIZE)
	{
		char n1 = *ip++;
		char n2 = *ip++;
		u8 val =
			 (n2 >= 'A' ? 10+n2-'A' : n2-'0') +
			((n1 >= 'A' ? 10+n1-'A' : n1-'0') << 4);
		*op-- = val;
		ip++;
		len++;
	}
	return (const u8 *) buf;
}



const char *getBTErrorString(u8 status)
{
	switch(status)
	{
		case 0x01 : return "Unknown HCI Command";
		case 0x02 : return "No Connection";
		case 0x03 : return "Hardware Failure";
		case 0x04 : return "Page Timeout";
		case 0x05 : return "Authentication Failure";
		case 0x06 : return "Key Missing";
		case 0x07 : return "Memory Full";
		case 0x08 : return "Connection Timeout";
		case 0x09 : return "Max Number Of Connections";
		case 0x0A : return "Max Number Of SCO Connections To A Device";
		case 0x0B : return "ACL Connection Already Exists";
		case 0x0C : return "Command Disallowed";
		case 0x0D : return "Host Rejected Due To Limited Resources";
		case 0x0E : return "Host Rejected Due To Security Reasons";
		case 0x0F : return "Host Rejected Due To A Remote Device Only A Personal Device";
		case 0x10 : return "Host Timeout";
		case 0x11 : return "Unsupported Feature Or Parameter Value";
		case 0x12 : return "Invalid HCI Command Parameters";
		case 0x13 : return "Other End Terminated Connection: User Ended Connection";
		case 0x14 : return "Other End Terminated Connection: Low Resources";
		case 0x15 : return "Other End Terminated Connection: About To Power Off";
		case 0x16 : return "Connection Terminated By Local Host";
		case 0x17 : return "Repeated Attempts";
		case 0x18 : return "Pairing Not Allowed";
		case 0x19 : return "Unknown LMP PDU";
		case 0x1A : return "Unsupported Remote Feature";
		case 0x1B : return "SCO Offset Rejected";
		case 0x1C : return "SCO Interval Rejected";
		case 0x1D : return "SCO Air Mode Rejected";
		case 0x1E : return "Invalid LMP Parameters";
		case 0x1F : return "Unspecified Error";
		case 0x20 : return "Unsupported LMP Parameter";
		case 0x21 : return "Role Change Not Allowed";
		case 0x22 : return "LMP Response Timeout";
		case 0x23 : return "LMP Error Transaction Collision";
		case 0x24 : return "LMP PDU Not Allowed";
		case 0x25 : return "Encryption Mode Not Acceptable";
		case 0x26 : return "Unit Key Used";
		case 0x27 : return "QoS Not Supported";
		case 0x28 : return "Instant Passed";
		case 0x29 : return "Pairing With Unit Key Not Supported";
	}
	return "reserved For Future Use";
}



const char *getBTEventName(u8 event_code)
{
	switch(event_code)
	{
		case 0x01 : return "EVENT_INQUIRY_COMPLETE";						
		case 0x02 : return "EVENT_INQUIRY_RESULT";					
		case 0x03 : return "EVENT_CONNECTION_COMPLETE";
		case 0x04 : return "EVENT_CONNECTION_REQUEST";				
		case 0x05 : return "EVENT_DISCONNECTION_COMPLETE";
		case 0x06 : return "EVENT_AUTHENTICATION_COMPLETE";			
		case 0x07 : return "EVENT_REMOTE_NAME_REQUEST_COMPLETE";
		case 0x08 : return "EVENT_ENCRYPTION_CHANGE";		
		case 0x09 : return "EVENT_CHANGE_LINK_KEY_COMPLETE";
		case 0x0A : return "EVENT_MASTER_LINK_KEY_COMPLETE";			
		case 0x0B : return "EVENT_READ_SUPPORTED_FEATURES_COMPLETE";
		case 0x0C : return "EVENT_READ_REMOTE_VERSION_COMPLETE";	
		case 0x0D : return "EVENT_Q0S_SETUP_COMPLETE";		
		case 0x0E : return "EVENT_COMMAND_COMPLETE";				
		case 0x0F : return "EVENT_COMMAND_STATUS";					
		case 0x10 : return "EVENT_HARDWARE_ERROR";					
		case 0x11 : return "EVENT_FLUSH_OCCURED";					
		case 0x12 : return "EVENT_ROLE_CHANGE";					
		case 0x13 : return "EVENT_NUMBER_OF_COMPLETED_PACKETS";
		case 0x14 : return "EVENT_MODE_CHANGE";		
		case 0x15 : return "EVENT_RETURN_LINK_KEYS";
		case 0x16 : return "EVENT_PIN_CODE_REQUEST";					
		case 0x17 : return "EVENT_LINK_KEY_REQUEST";					
		case 0x18 : return "EVENT_LINK_KEY_NOTIFICATION";
		case 0x19 : return "EVENT_LOOPBACK_COMMAND";			
		case 0x1A : return "EVENT_DATA_BUFFER_OVERFLOW";
		case 0x1B : return "EVENT_MAX_SLOTS_CHANGE";				
		case 0x1C : return "EVENT_READ_CLOCK_OFFSET_COMPLETE";
		case 0x1D : return "EVENT_CONNECTION_PACKET_TYPE_CHANGED";
		case 0x1E : return "EVENT_QOS_VIOLATION";	
		case 0x1F : return "EVENT_PAGE_SCAN_MODE_CHANGE";
		case 0x20 : return "EVENT_PAGE_SCAN_REPETITION_MODE_CHANGE";
	}
	return "unknown EVENT_CODE";
}


//--------------------------------
// processCommandCompleteEvent()
//--------------------------------
// This method creates a very specific sequence of events
// during initialization. As we receive each COMMAND_COMPLETE
// complete event, we send a subsequent HCI_OP_BASEBAND command,
// so that the events occur exactly in the order they are liste
// in the switch() statement, until finally we set "m_is_setup"
// true, which allows the HCI layer wait loop in initialize()
// to continue ...
//
// So when looking for the code where we send out a particular command,
// remember that it is in the switch statement case for the PREVIOUS
// command_completed event ..


bool hciBase::processCommandCompleteEvent(const void *buffer, u16 length)
{
	assert(length >= sizeof(hci_event_header));
	hci_command_complete_event *pCommandComplete = (hci_command_complete_event *) buffer;
	
	switch (pCommandComplete->command_opcode)
	{
		// gets here from a call to ::setup() ...
		
		case HCI_OP_INFORM_READ_BD_ADDR:
		{
			assert(length >= sizeof(hci_read_bdaddr_complete_event));
			hci_read_bdaddr_complete_event *pEvent = (hci_read_bdaddr_complete_event *) buffer;
			memcpy(m_local_addr, pEvent->addr, BT_ADDR_SIZE);

			LOG("Local BT Address: %02X:%02X:%02X:%02X:%02X:%02X",
				(unsigned) m_local_addr[5],
				(unsigned) m_local_addr[4],
				(unsigned) m_local_addr[3],
				(unsigned) m_local_addr[2],
				(unsigned) m_local_addr[1],
				(unsigned) m_local_addr[0]);
			LOG("Local Device Class: %06x",m_device_class);
					// we only send out 3 bytes
					
			hci_write_class_of_device_command cmd;
			cmd.header.opcode = HCI_OP_BASEBAND_WRITE_CLASS_OF_DEVICE;
			cmd.class_of_device[0] = m_device_class       & 0xFF;
			cmd.class_of_device[1] = m_device_class >> 8  & 0xFF;
			cmd.class_of_device[2] = m_device_class >> 16 & 0xFF;
			m_pHCI->sendCommand(&cmd, sizeof cmd);
			return true;
		}


		case HCI_OP_BASEBAND_WRITE_CLASS_OF_DEVICE:
		{
			// LOG("finished WRITE_CLASS_OF_DEVICE",0);
			LOG("Local Name: '%s'",m_local_name);
			hci_write_local_name_command cmd;
			cmd.header.opcode = HCI_OP_BASEBAND_WRITE_LOCAL_NAME;
			memcpy(cmd.local_name, m_local_name, sizeof cmd.local_name);
			m_pHCI->sendCommand(&cmd, sizeof cmd);
			return true;
		}


	#if DEBUG_ADDL_CONTROLLER_INFO		
			
		case HCI_OP_BASEBAND_WRITE_LOCAL_NAME:
		{
			// LOG("finished WRITE_LOCAL_NAME",0);
			hci_command_header cmd;
			cmd.opcode = HCI_OP_INFORM_READ_BUFFER_SIZE;
			cmd.length = 0;
			m_pHCI->sendCommand(&cmd, 3);
			return true;
		}
		case HCI_OP_INFORM_READ_BUFFER_SIZE:
		{
			assert(length >= sizeof(hci_read_buffer_sizes_complete_event));
			hci_read_buffer_sizes_complete_event *pEvent = (hci_read_buffer_sizes_complete_event *) buffer;
			
			LOG("buffer SIZES acl(%d) sync(%d) NUM acl(%d) sync(%d)",
				pEvent->acl_data_packet_length,
				pEvent->synchronous_data_packet_length,
				pEvent->total_num_acl_data_packets,
				pEvent->total_num_synchronous_data_packets);

			hci_command_header cmd;
			cmd.opcode = HCI_OP_INFORM_READ_LOCAL_SUPPORTED_COMMANDS;
			cmd.length = 0;
			m_pHCI->sendCommand(&cmd, 3);
			return true;
		}
		case HCI_OP_INFORM_READ_LOCAL_SUPPORTED_COMMANDS:
		{
			assert(pCommandComplete->header.length > 4);
				// or it didn't give us any information ...
			display_bytes("buffer",(u8 *)buffer,length);
			
			u8 *p = pCommandComplete->param;
			LOG("Info: supported commands",0);
			for (u16 i=0; i<pCommandComplete->header.length - 4; i++)
				showSupportedCommand(i,*p++);
			
			hci_command_header cmd;
			cmd.opcode = HCI_OP_INFORM_READ_LOCAL_SUPPORTED_FEATURES;
			cmd.length = 0;
			m_pHCI->sendCommand(&cmd, 3);
			return true;
		}
		case HCI_OP_INFORM_READ_LOCAL_SUPPORTED_FEATURES:
		{
			LOG("Info: supported features",0);
			display_bytes("features",
				pCommandComplete->param,
				pCommandComplete->header.length - 4);

			hci_set_event_mask_command cmd;
			cmd.header.opcode = HCI_OP_BASEBAND_SET_EVENT_MASK;
			cmd.event_mask[0] = 0xffff;
			cmd.event_mask[1] = 0xffff;
			cmd.event_mask[2] = 0xffff;
			cmd.event_mask[3] = 0xffff;
			display_bytes("event mask",(u8 *)&cmd,sizeof(cmd));
			m_pHCI->sendCommand(&cmd, sizeof cmd);
			return true;
		}
		case HCI_OP_BASEBAND_SET_EVENT_MASK:
		{

				
	#else
	
		case HCI_OP_BASEBAND_WRITE_LOCAL_NAME:
		{
		
	#endif
	
	#if WRITE_EIR			// not supported on my USB controller
	
			if (!m_pHCI->getTransport()->isUartTransport())
				goto dont_write_eir;

			// we program the hardware to broadcast some
			// useful service UUIDs or, although we can pair,
			// nobody will otherwise pay attention to us (we
			// can't "do" anything unless we write this)
			//
			// Properly, higher level services should register
			// their UUIDs with this HCI (base) layer for this
			// "broadcast" ... but for now I'm just plugging in
			// some known UUIDs.
			
			// The HCI command itself is just a byte, followed by data.
			// byte0 == "FEC encoding is required"  0 or 1. The
			// following bytes are the extended inquiry response
			// we want the radio to send out ..
			//
			// see gap_defs.h
			
			hci_write_extended_inquiry_response_command cmd;
				// prh - may return a different event than command completed:
				// HCI_Write_Extended_Inquiry_Response command succeeded
			cmd.header.opcode = HCI_OP_WRITE_EXTENDED_INQUIRY_RESPONSE;
			cmd.fec_encoding_required = 1;
				// Note that we don't do "extended inquiries" :
				// 		"[The extended inquiry response] event is only generated if the
				//		 Inquiry_Mode parameter of the last HCI_Write_Inquiry_Mode command
				//		 was set to 0x02 (Inquiry Result with RSSI format or Extended
				//		 Inquiry Result format)."
				// This also indicates that we need to set THIS bit to differentiate
				//    the packet from an "RSSI format" inquiry result packet
			memset(cmd.data,0,240);
			u8 *p = cmd.data;

			// let's put our full name in the packet
			
			#if 1
				u8 name_len = strlen((const char *)m_local_name);
				SET_BYTE(p,GAP_AD_TYPE_COMPLETE_NAME);			// type
				SET_BYTE(p,name_len);							// len
				SET_COPY(p,m_local_name,name_len);				// data
			#endif
			
			// Add some services
			// not sure if I should send out a single list, or multiple single items
			// I thought I would use GAP_AD_TYPE_SERVICE_16_BIT_DATA
			// but the example I found on the net used GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID
			// for services ...
			
			#if 1
				SET_BYTE(p,GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID);	// type
				SET_BYTE(p,4);									// len = 4 bytes == 2 16 bit service IDs
				//	SET_BYTE(p,5);		// THE MAGIC "05" goes here
				SET_WORD(p,be(SDP_SERVER_SVCLASS_ID));				// data
				SET_WORD(p,be(SERIAL_PORT_SVCLASS_ID));				// data
			#endif
			
			// add some protocols?  dont think so ...
			
			#if 1
				SET_BYTE(p,GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID);	// type
				SET_BYTE(p,6);								// len = 4 bytes == 3 16 bit uuids
				SET_WORD(p,SDP_UUID);						// data
				SET_WORD(p,RFCOMM_UUID);					// data
				SET_WORD(p,L2CAP_UUID);						// data
			#endif
			
			LOG("Setting EIR ServiceIDs and protocol UUIDs",0);
			display_bytes("cmd",(u8 *)&cmd,sizeof(cmd));
			m_pHCI->sendCommand(&cmd, sizeof cmd);
			return true;
		}

			
			
		//--------------------------------------------------------
		//
		// "The extended inquiry response data format is shown in Figure 8.1. The data is
		//	240 octets and consists of a significant part and a non-significant part. The
		//	significant part contains a sequence of data structures. Each data structure
		//	shall have a length field of one octet, which contains the Length value, and a
		//	data field of Length octets. The first n octets of the data field contain the
		//	extended inquiry response (EIR) data type. The content of the remaining
		//	Length - n octets in the data field depends on the value of the EIR data type
		//	and is called the EIR data. The non-significant part extends the extended
		//	inquiry response to 240 octets and shall contain all-zero octets."
		//
		// "The extended inquiry response data formats and meanings are defined in
		//	[Core Specification Supplement], Part A, Section 1. The extended inquiry
		//	response data type values are defined in [Assigned Numbers]."
		//
		// Yeah, "simple protocol for ..." ... the [supplement] is an additional PDF
		// document.  [Assigned Numbers] is a live webpage listing various constants that
		// are defined, and growing, for BT ...
		//
		// From the supplement, we can find that for EIR, there are the following optional
		// data types:
		//
		//			Service UUID (multiple)
		//			Local Name (single)
		//			Flags (single)
		//			Manufacturer Specific Data (multiple)
		//          TX Power Level (multiple)
		// 			Uniform Resource Identifier (multiple)
		//
		// "The values for the data types are listed in Assigned Numbers."
		// The phrase "EIR data types" appears exactly 3 times in the core spec,
		// and nowhere can I find the "type" bytes for the "first n octets".
		//
		// So, we go to the Assigned NUmber webpage.  Nowhere do is see "EIR data types" or
		// "data types". Look under "baseband".  Oh ... those are the "values" ... not the "types".
		// Hard to find the bits ... what are the "type" bytes for the "first n octets
		// of the data field contain the extended inquiry response (EIR) data type"??????
		//
		// So, after looking at the supplement, and the webpage, I guess I go back to
		// the 15,000 page core specification and look for an indication of what
		// f'ing bytes I'm supposed to send. Or I could save time (NOT!) by trying
		// to find an example of a packet that somebody else has created, and then
		// try to start reverse engineering existing BT stack source code, FFS.
		//
		// No joy in the core spec.  So I google "extended inquiry response EIR data type",
		// find a webpage from some other guy who tried to figure this out. He
		// shows some constants like
		//
		//			GAP_AD_TYPE_FLAGS 				0x01
		// 			GAP_AD_TYPE_SHORTENED_NAME 		0x08
		//          GAP_AD_TYPE_MANU_SPECIFIC_DATA 	0xff
		//
		// From which I infer that everybody calls these the "GAP Advertising Types"
		// which I then google, and I get "All AD types are listed in the Bluetooth
		// Assigned Numbers document", so I go back to that ... FROM WHICH I FINALLY
		// FIND, hidden under the main webpage, on the "Generic Access Profile"
		// list of assigned numbers, this:
		//
		// "Generic Access Profile
		//	Assigned numbers are used in GAP for inquiry response,
		//	EIR data type values, manufacturer-specific data, advertising data,
		//	low energy UUIDs and appearance characteristics, and class of device."
		//
		// and a list of the values to figure out a single "type" byte.
		// This is an example of what I've had to do over and over again
		// through this process due to the horrible, yet, vast, bluetooth
		// specification. Read the docs, look for examples, connect terminology,
		// and suss out wtf they are talking about.
		//
		// I look thru my h files and see I don't have anything that looks like
		// these numbers, so either I create my own H file, or I try to find one
		// that already exists.  Since there are likely other subtypes, I will
		// try to find one that already exists.  Google GAP_AD_TYPE_FLAGS,
		//
		// I find some.  Here's what I don't get.  Why does everybody have to
		// be license-restricted to copying fucking constants defined publicly
		// in these specs ... another money making scheme, I guess. Stupid.
		// There should have been a well defined, common set of public H files
		// someplace with no license restictions.  FFS, they're constants.
		//==============================================================
		// Did they say this was simple?   Yes they f'ing did.
		// it no longer pairs with android, which now gives an
		//      unhandled EVENT_AUTHENTICATION_COMPLETE event
		// android "force pair" app, still shows "waiting for
		//     supported uuid" and android disconnects us when
		//     we try to pair.
		// seems to take longer in windows
		//     note that it comes up as "unknown" device, then
		//     presumably does a name inquiry to get the name,
		//     so I'm not even sure the basics are working here.
		//
		// found what I *think* is an example on the net:
		//
		// 0B0954657374206E65787573170305110A110C110E111211151116111F112F1100123211010501070000000000000000000000000000000
		//
		// 			len  type    data 
		// 			0B   09		 54 65 73 74 20 6E 65 78 75 73
		// 			17   03  05  110A 110C 110E 1112 1115
		// 			         ^   1116 111F 112F 1100 1232
		// 	 		         |   1101
		//  wtf is this? ----+          v  above uuids  ^
		//  						AUDIO_SOURCE_SVCLASS_ID
		//  						AV_REMOTE_TARGET_SVCLASS_ID
		//  						AV_REMOTE_SVCLASS_ID
		//							HEADSET_AGW_SVCLASS_ID
		//							PANU_SVCLASS_ID
		//						
		//							NAP_SVCLASS_ID
		//  						HANDSFREE_AGW_SVCLASS_ID
		//  						PBAP_PSE_SVCLASS_ID
		//  						INTERCOM_SVCLASS_ID
		// 							MAP_MSE_SVCLASS_ID
		//						
		//  						SERIAL_PORT_SVCLASS_ID
		//
		// 			len  type    data  (continuing)
		//          05	 01      07 00 00 00 00 				flags
		//
		//          FLAGS (all are in octet[0], the other 4 bytes are reserved)
		//
		//          	bit 
		//				0 		LE Limited Discoverable Mode
		//				1 		LE General Discoverable Mode
		//				2 		BR/EDR Not Supported. Bit 37 of LMP
		//						Feature Mask Definitions (Page 0)
		//				3 		Simultaneous LE and BR/EDR to Same
		//						Device Capable (Controller). Bit 49 of
		//						LMP Feature Mask Definitions (Page 0)
		//				4 		Simultaneous LE and BR/EDR to Same
		//						Device Capable (Host). Bit 66 of LMP
		//						Feature Mask Definitions (Page 1)
		//
		//			None of which seem to apply to me (rPi is BR/EDR
		//          only, no BT LE)
		//
		//  There's another magic byte:  the "wtf is this" "05".
		//  Can't find "Complete List of 16-bit Service Class UUID"
		//  in core spec. the The "assigned numbers" webpage refers
		//  Maybe it's just an LE specific byte?
		//
		//	"Bluetooth Core Specification:Vol. 3, Part C, section 8.1.1
		//   (v2.1 + EDR, 3.0 + HS and 4.0)Vol. 3, Part C, sections 11.1.1
		//   and 18.2 (v4.0)Core Specification Supplement, Part A, section
		//   1.1"
		//
		// In Android "BT Device Information" app
		// LENOVO shows uuids  HFP,A2DP,AVRCP+TG,SPP,PANU, and Generic Audio
		// 111f,110a,110c,110e,1101,1115, and 1203.  I show nuthin ..
		// well, that's not true, actually, I can no longer pair ..
		// scuse me, gotta go handle EVENT_AUTHENTICATION_COMPLETE
		//
		// It pairs fine, found quickly, on BLU
		//
		//==============================================================
		
		
		case HCI_OP_WRITE_EXTENDED_INQUIRY_RESPONSE:
		{
			
dont_write_eir:			
			
	#endif	// WRITE_EIR
	
			// SCAN_ENABLE turns the radio on!
			// So we are effectively "started" after this command
			
			hci_write_scan_enable_command cmd;
			cmd.header.opcode = HCI_OP_BASEBAND_WRITE_SCAN_ENABLE;
			cmd.scan_enable = HCI_LINK_SCAN_ENABLE_BOTH_ENABLED;
			m_pHCI->sendCommand(&cmd, sizeof cmd);
			return true;
		}
		
		case HCI_OP_BASEBAND_WRITE_SCAN_ENABLE:
			// LOG("finished WRITE_SCAN_ENABLE",0);
			LOG("setup complete",0);
			m_is_setup = true;
			return true;
	
	}	// switch
	
	return false;	// the event was not handled
}

