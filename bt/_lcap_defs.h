
#ifndef __LCAP_DEFS_H
#define __LCAP_DEFS_H

#include "_hci_defs.h"

// The layer handles any requests that come in on 
// the LCAP_SIGNAL_CID channel. Others are passed up.

#define LCAP_SIGNAL_CID 0x0001

//-----------------------------------------
// PSM (Protocol Service Multiplexers)
//-----------------------------------------

#define SDP_PSM		    0x0001
#define RFCOMM_PSM		0x0003


//-----------------------------------------
// L2CAP Signaling Commands
//-----------------------------------------

#define LCAP_COMMAND_REJECT 			    0x01
#define LCAP_CONNECTION_REQUEST			    0x02
#define LCAP_CONNECTION_RESPONSE       	    0x03
#define LCAP_CONFIGURE_REQUEST         	    0x04
#define LCAP_CONFIGURE_RESPONSE        	    0x05
#define LCAP_DISCONNECTION_REQUEST     	    0x06
#define LCAP_DISCONNECTION_RESPONSE    	    0x07
#define LCAP_ECHO_REQUEST              	    0x08
#define LCAP_ECHO_RESPONSE             	    0x09
#define LCAP_INFORMATION_REQUEST       	    0x0a
#define LCAP_INFORMATION_RESPONSE      	    0x0b
/* 0x0c - 0x11 used for AMP */
#define LCAP_PARAMETER_UPDATE_REQUEST 		0x12
#define LCAP_PARAMETER_UPDATE_RESPONSE		0x13
#define LCAP_LE_CONNECTION_REQUEST         	0x14
#define LCAP_LE_CONNECTION_RESPONSE       	0x15
#define LCAP_LE_FLOW_CONTROL_CREDIT       	0x16


// LCAP_FEATURES_SUPPORTED
// packed into 16 bit word, delivered LSB first
// followed by two zeros

#define LCAP_FEATURE_FLOW_CONTROL_MODE              0x0001    // (1 << (0 + 0))
#define LCAP_FEATURE_RETRANSMISSION_MODE            0x0002    // (1 << (0 + 1))
#define LCAP_FEATURE_BI_DIRECTIONAL_QOS             0x0004    // (1 << (0 + 2))
#define LCAP_FEATURE_ENHANCED_RETRANSMISSION_MODE   0x0008    // (1 << (0 + 3))
#define LCAP_FEATURE_STREAMING_MODE                 0x0010    // (1 << (0 + 4))
#define LCAP_FEATURE_FCS_OPTION                     0x0020    // (1 << (0 + 5))
#define LCAP_FEATURE_EXTENDED_FLOW_SPECIFICATIONR   0x0040    // (1 << (0 + 6))
#define LCAP_FEATURE_FIXED_CHANNELS                 0x0080    // (1 << (0 + 7))
#define LCAP_FEATURE_EXTENDED_WINDOW_SIZE           0x0100    // (1 << (8 + 0))
#define LCAP_FEATURE_UNI_CLESS_DATA_RECEPTION       0x0200    // (1 << (8 + 1))


//--------------------------------------------
// my LCAP structure definitions
//--------------------------------------------

struct lcap_data_header
{
	u16 len;
	u16 cid;
}	PACKED;

struct lcap_command_header
{
	u8  cmd;
	u8  id;
	u16 cmd_len;
}	PACKED;


struct lcap_data_packet_header
{
    hci_data_header  hci;    
    lcap_data_header lcap_d;
} PACKED;


struct lcap_command_packet_header
{
    hci_data_header  hci;    
    lcap_data_header lcap_d;
    lcap_command_header lcap_c;
} PACKED;





struct lcap_connect_request
{
    lcap_command_packet_header hdr;
    u16 psm;
	u16 src_cid;
}	PACKED;

struct lcap_connect_response
{
    lcap_command_packet_header hdr;
	u16 dest_cid;
	u16 src_cid;
	u16 result;
	u16 status;
}	PACKED;

struct lcap_config_request
    // sized for an outgoing MTU config request
{
    lcap_command_packet_header hdr;
	u16 dest_cid;
	u16 flags;
	u8 options[4];
}	PACKED;

struct lcap_config_response
{
    lcap_command_packet_header hdr;
	u16 dest_cid;
	u16 flags;
	u16 result;
}	PACKED;


struct lcap_info_request
{
    lcap_command_packet_header hdr;
	u16 info_type;
} 	PACKED;


struct lcap_info_response_features
{
    lcap_command_packet_header hdr;
	u16 info_type;
	u16 result;
	u8  data[4];
}	PACKED;


struct lcap_info_response_fixed_channels
{
    lcap_command_packet_header hdr;
	u16 info_type;
	u16 result;
	u8  data[8];
}	PACKED;


struct lcap_command_reject
{
    lcap_command_packet_header hdr;
	u16 reason;
	union {
		u16 max_mtu;
		struct {
			u16 lcid;
			u16 rcid;
		};
	};
}   PACKED;

struct lcap_disconnect_request
{
    lcap_command_packet_header hdr;
	u16 dest_cid;
	u16 src_cid;
}   PACKED;




#endif 
