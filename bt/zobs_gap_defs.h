
#ifndef __GAP_DEFS_H_
#define __GAP_DEFS_H_

//--------------------------------------------
// Assigned numbers
//--------------------------------------------

#define SDP_PSM		0x0001

// Protocol UUIDs
 
#define UUID_PROTO_SDP	        0x0001
#define UUID_PROTO_UDP	        0x0002
#define UUID_PROTO_RFCOMM	    0x0003
#define UUID_PROTO_TCP	        0x0004
#define UUID_PROTO_TCS_BIN	    0x0005
#define UUID_PROTO_TCS_AT	    0x0006
#define UUID_PROTO_ATT	        0x0007
#define UUID_PROTO_OBEX	        0x0008
#define UUID_PROTO_IP		    0x0009
#define UUID_PROTO_FTP	        0x000a
#define UUID_PROTO_HTTP	        0x000c
#define UUID_PROTO_WSP	        0x000e
#define UUID_PROTO_BNEP	        0x000f
#define UUID_PROTO_UPNP	        0x0010
#define UUID_PROTO_HIDP	        0x0011
#define UUID_PROTO_HCRP_CTRL	0x0012
#define UUID_PROTO_HCRP_DATA	0x0014
#define UUID_PROTO_HCRP_NOTE	0x0016
#define UUID_PROTO_AVCTP	    0x0017
#define UUID_PROTO_AVDTP	    0x0019
#define UUID_PROTO_CMTP	        0x001b
#define UUID_PROTO_UDI	        0x001d
#define UUID_PROTO_MCAP_CTRL	0x001e
#define UUID_PROTO_MCAP_DATA	0x001f
#define UUID_PROTO_L2CAP	    0x0100




// Service class identifiers of standard services and service groups

#define SDP_SERVER_SVCLASS_ID		        0x1000
#define BROWSE_GRP_DESC_SVCLASS_ID	        0x1001
#define PUBLIC_BROWSE_GROUP		            0x1002
#define SERIAL_PORT_SVCLASS_ID		        0x1101
#define LAN_ACCESS_SVCLASS_ID		        0x1102
#define DIALUP_NET_SVCLASS_ID		        0x1103
#define IRMC_SYNC_SVCLASS_ID		        0x1104
#define OBEX_OBJPUSH_SVCLASS_ID		        0x1105
#define OBEX_FILETRANS_SVCLASS_ID	        0x1106
#define IRMC_SYNC_CMD_SVCLASS_ID	        0x1107
#define HEADSET_SVCLASS_ID		            0x1108
#define CORDLESS_TELEPHONY_SVCLASS_ID	    0x1109
#define AUDIO_SOURCE_SVCLASS_ID		        0x110a
#define AUDIO_SINK_SVCLASS_ID		        0x110b
#define AV_REMOTE_TARGET_SVCLASS_ID	        0x110c
#define ADVANCED_AUDIO_SVCLASS_ID	        0x110d
#define AV_REMOTE_SVCLASS_ID		        0x110e
#define AV_REMOTE_CONTROLLER_SVCLASS_ID	    0x110f
#define INTERCOM_SVCLASS_ID		            0x1110
#define FAX_SVCLASS_ID			            0x1111
#define HEADSET_AGW_SVCLASS_ID		        0x1112
#define WAP_SVCLASS_ID			            0x1113
#define WAP_CLIENT_SVCLASS_ID		        0x1114
#define PANU_SVCLASS_ID			            0x1115
#define NAP_SVCLASS_ID			            0x1116
#define GN_SVCLASS_ID			            0x1117
#define DIRECT_PRINTING_SVCLASS_ID	        0x1118
#define REFERENCE_PRINTING_SVCLASS_ID	    0x1119
#define IMAGING_SVCLASS_ID		            0x111a
#define IMAGING_RESPONDER_SVCLASS_ID	    0x111b
#define IMAGING_ARCHIVE_SVCLASS_ID	        0x111c
#define IMAGING_REFOBJS_SVCLASS_ID	        0x111d
#define HANDSFREE_SVCLASS_ID		        0x111e
#define HANDSFREE_AGW_SVCLASS_ID	        0x111f
#define DIRECT_PRT_REFOBJS_SVCLASS_ID	    0x1120
#define REFLECTED_UI_SVCLASS_ID		        0x1121
#define BASIC_PRINTING_SVCLASS_ID	        0x1122
#define PRINTING_STATUS_SVCLASS_ID	        0x1123
#define HID_SVCLASS_ID			            0x1124
#define HCR_SVCLASS_ID			            0x1125
#define HCR_PRINT_SVCLASS_ID		        0x1126
#define HCR_SCAN_SVCLASS_ID		            0x1127
#define CIP_SVCLASS_ID			            0x1128
#define VIDEO_CONF_GW_SVCLASS_ID	        0x1129
#define UDI_MT_SVCLASS_ID		            0x112a
#define UDI_TA_SVCLASS_ID		            0x112b
#define AV_SVCLASS_ID			            0x112c
#define SAP_SVCLASS_ID			            0x112d
#define PBAP_PCE_SVCLASS_ID		            0x112e
#define PBAP_PSE_SVCLASS_ID		            0x112f
#define PBAP_SVCLASS_ID			            0x1130

#define MAP_MSE_SVCLASS_ID		            0x1132
#define MAP_MCE_SVCLASS_ID		            0x1133
#define MAP_SVCLASS_ID			            0x1134
#define GNSS_SVCLASS_ID			            0x1135
#define GNSS_SERVER_SVCLASS_ID		        0x1136

#define PNP_INFO_SVCLASS_ID		            0x1200
#define GENERIC_NETWORKING_SVCLASS_ID	    0x1201
#define GENERIC_FILETRANS_SVCLASS_ID	    0x1202
#define GENERIC_AUDIO_SVCLASS_ID	        0x1203
#define GENERIC_TELEPHONY_SVCLASS_ID	    0x1204
#define UPNP_SVCLASS_ID			            0x1205
#define UPNP_IP_SVCLASS_ID		            0x1206
#define UPNP_PAN_SVCLASS_ID		            0x1300
#define UPNP_LAP_SVCLASS_ID		            0x1301
#define UPNP_L2CAP_SVCLASS_ID		        0x1302
#define VIDEO_SOURCE_SVCLASS_ID		        0x1303
#define VIDEO_SINK_SVCLASS_ID		        0x1304
#define VIDEO_DISTRIBUTION_SVCLASS_ID	    0x1305

#define HDP_SVCLASS_ID			            0x1400
#define HDP_SOURCE_SVCLASS_ID		        0x1401
#define HDP_SINK_SVCLASS_ID		            0x1402

#define APPLE_AGENT_SVCLASS_ID		        0x2112


#define GENERIC_ATTRIB_SVCLASS_ID	        0x1801





//-------------------------------------------
// Gap defines not really used
//-------------------------------------------
enum
{
    GAP_AD_TYPE_FLAGS                      = 0x01,
        // Flag
    GAP_AD_TYPE_MORE_16_BIT_UUID           = 0x02,
        // Use of more than 16 bits UUID
    GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID  = 0x03,
        // Complete list of 16 bit UUID
    GAP_AD_TYPE_MORE_32_BIT_UUID           = 0x04,
        // Use of more than 32 bit UUD
    GAP_AD_TYPE_COMPLETE_LIST_32_BIT_UUID  = 0x05,
        // Complete list of 32 bit UUID
    GAP_AD_TYPE_MORE_128_BIT_UUID          = 0x06,
        // Use of more than 128 bit UUID
    GAP_AD_TYPE_COMPLETE_LIST_128_BIT_UUID = 0x07,
        // Complete list of 128 bit UUID
    GAP_AD_TYPE_SHORTENED_NAME             = 0x08,
        // Shortened device name
    GAP_AD_TYPE_COMPLETE_NAME              = 0x09,
        // Complete device name
    GAP_AD_TYPE_TRANSMIT_POWER             = 0x0A,
        // Transmit power
    GAP_AD_TYPE_CLASS_OF_DEVICE            = 0x0D,
        // Class of device
    GAP_AD_TYPE_SP_HASH_C                  = 0x0E,
        // Simple Pairing Hash C
    GAP_AD_TYPE_SP_RANDOMIZER_R            = 0x0F,
        // Simple Pairing Randomizer
    GAP_AD_TYPE_TK_VALUE                   = 0x10,
        // Temporary key value
    GAP_AD_TYPE_OOB_FLAGS                  = 0x11,
        // Out of Band Flag
    GAP_AD_TYPE_SLAVE_CONN_INT_RANGE       = 0x12,
        // Slave connection interval range
    GAP_AD_TYPE_RQRD_16_BIT_SVC_UUID       = 0x14,
        // Require 16 bit service UUID
    GAP_AD_TYPE_RQRD_32_BIT_SVC_UUID       = 0x1F,
        // Require 32 bit service UUID
    GAP_AD_TYPE_RQRD_128_BIT_SVC_UUID      = 0x15,
        // Require 128 bit service UUID
    GAP_AD_TYPE_SERVICE_16_BIT_DATA        = 0x16,
        // Service data 16-bit UUID
    GAP_AD_TYPE_SERVICE_32_BIT_DATA        = 0x20,
        // Service data 32-bit UUID
    GAP_AD_TYPE_SERVICE_128_BIT_DATA       = 0x21,
        // Service data 128-bit UUID
    GAP_AD_TYPE_PUB_TGT_ADDR               = 0x17,
        // Public Target Address
    GAP_AD_TYPE_RAND_TGT_ADDR              = 0x18,
        // Random Target Address
    GAP_AD_TYPE_APPEARANCE                 = 0x19,
        // Appearance
    GAP_AD_TYPE_ADV_INTV                   = 0x1A,
        // Advertising Interval
    GAP_AD_TYPE_LE_BT_ADDR                 = 0x1B,
        // LE Bluetooth Device Address
    GAP_AD_TYPE_LE_ROLE                    = 0x1C,
        // LE Role
    GAP_AD_TYPE_SPAIR_HASH                 = 0x1D,
        // Simple Pairing Hash C-256
    GAP_AD_TYPE_SPAIR_RAND                 = 0x1E,
        // Simple Pairing Randomizer R-256
    GAP_AD_TYPE_3D_INFO                    = 0x3D,
        // 3D Information Data
    GAP_AD_TYPE_MANU_SPECIFIC_DATA         = 0xFF,
        // Manufacturer specific data
};
    

#if 0
    // Random Address type
    enum gap_rnd_addr_type
    {
        GAP_STATIC_ADDR     = SMPM_ADDR_TYPE_STATIC,
            // Static random address
        GAP_NON_RSLV_ADDR   = SMPM_ADDR_TYPE_PRIV_NON_RESOLV,
            // Private non resolvable address
        GAP_RSLV_ADDR       = SMPM_ADDR_TYPE_PRIV_RESOLV,
            // Private resolvable address
    };
#endif


// GAP Specific Error

enum gap_err_code
{
    GAP_ERR_NO_ERROR        = 0x00,
        // No error
    GAP_ERR_INVALID_PARAM   = 0x40,
        // Invalid parameters set
    GAP_ERR_PROTOCOL_PROBLEM,
        // Problem with protocol exchange, get unexpected response
    GAP_ERR_NOT_SUPPORTED,
        // Request not supported by software configuration
    GAP_ERR_COMMAND_DISALLOWED,
        // Request not allowed in current state.
    GAP_ERR_CANCELED,
        // Requested operation canceled.
    GAP_ERR_TIMEOUT,
        // Requested operation timeout.
    GAP_ERR_DISCONNECTED,
        // Link connection lost during operation.
    GAP_ERR_NOT_FOUND,
        // Search algorithm finished, but no result found
    GAP_ERR_REJECTED,
        // Request rejected by peer device
    GAP_ERR_PRIVACY_CFG_PB,
        // Problem with privacy configuration
    GAP_ERR_ADV_DATA_INVALID,
        // Duplicate or invalid advertising data
};


// Boolean value set

enum
{
    GAP_DISABLE = 0x00,
        // Disable
    GAP_ENABLE
        // Enable
};



#if 0   // prh (BLE_ATTS)
// GAP Attribute database handles
// Generic Access Profile Service
enum
{
    GAP_IDX_PRIM_SVC,
    GAP_IDX_CHAR_DEVNAME,
    GAP_IDX_DEVNAME,
    GAP_IDX_CHAR_ICON,
    GAP_IDX_ICON,
    #if (BLE_PERIPHERAL)
    GAP_IDX_CHAR_PRIVY_FLAG,
    GAP_IDX_PRIVY_FLAG,
    GAP_IDX_CHAR_SLAVE_PREF_PARAM,
    GAP_IDX_SLAVE_PREF_PARAM,
    GAP_IDX_CHAR_RECON_ADDR,
    GAP_IDX_RECON_ADDR,
    #endif /* #if (BLE_PERIPHERAL) */
    GAP_IDX_NUMBER,
};
#endif /* (BLE_ATTS)*/



// GAP Role 

enum gap_role
{
    GAP_NO_ROLE    = 0x00,
        // No role set yet
    GAP_OBSERVER_SCA    = 0x01,
        // Observer role
    GAP_BROADCASTER_ADV = 0x02,
        // Broadcaster role
    GAP_CENTRAL_MST     = (0x04 | GAP_OBSERVER_SCA),
        // Master/Central role
    GAP_PERIPHERAL_SLV  = (0x08 | GAP_BROADCASTER_ADV),
        // Peripheral/Slave role
};


// Advertising mode

enum gap_adv_mode
{
    GAP_NON_DISCOVERABLE,
        // Mode in non-discoverable
    GAP_GEN_DISCOVERABLE,
        // Mode in general discoverable
    GAP_LIM_DISCOVERABLE,
        // Mode in limited discoverable
    GAP_BROADCASTER_MODE,
        // Broadcaster mode which is a non discoverable and non connectable mode.
};


// Scan mode

enum gap_scan_mode
{
    GAP_GEN_DISCOVERY,
        // Mode in general discovery
    GAP_LIM_DISCOVERY,
        // Mode in limited discovery
    GAP_OBSERVER_MODE,
        // Observer mode
    GAP_INVALID_MODE,
        // Invalid mode
};



// IO Capability Values

enum gap_io_cap
{
    GAP_IO_CAP_DISPLAY_ONLY = 0x00,
        // Display Only
    GAP_IO_CAP_DISPLAY_YES_NO,
        // Display Yes No
    GAP_IO_CAP_KB_ONLY,
        // Keyboard Only
    GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
        // No Input No Output
    GAP_IO_CAP_KB_DISPLAY,
        // Keyboard Display
    GAP_IO_CAP_LAST,
};


// TK Type

enum gap_tk_type
{
    GAP_TK_OOB         = 0x00,
        //  TK get from out of band method
    GAP_TK_DISPLAY,
        // TK generated and shall be displayed by local device
    GAP_TK_KEY_ENTRY,
        // TK shall be entered by user using device keyboard
};


// OOB Data Present Flag Values

enum gap_oob
{
    GAP_OOB_AUTH_DATA_NOT_PRESENT = 0x00,
        // OOB Data not present
    GAP_OOB_AUTH_DATA_PRESENT,
        // OOB data present
    GAP_OOB_AUTH_DATA_LAST,
};


// Authentication mask

enum gap_auth_mask
{
    GAP_AUTH_NONE = 0,
        // No Flag set
    GAP_AUTH_BOND = (1 << 0),
        // Bond authentication
    GAP_AUTH_MITM = (1 << 2),
        // Man In the middle protection
};


// Authentication Requirements

enum gap_auth
{
    GAP_AUTH_REQ_NO_MITM_NO_BOND = (GAP_AUTH_NONE),
        // No MITM No Bonding
    GAP_AUTH_REQ_NO_MITM_BOND    = (GAP_AUTH_BOND),
        // No MITM Bonding
    GAP_AUTH_REQ_MITM_NO_BOND    = (GAP_AUTH_MITM),
        // MITM No Bonding
    GAP_AUTH_REQ_MITM_BOND       = (GAP_AUTH_MITM | GAP_AUTH_BOND),
        // MITM and Bonding
    GAP_AUTH_REQ_LAST
};


// Key Distribution Flags

enum gap_kdist
{
    GAP_KDIST_NONE = 0x00,
        // No Keys to distribute
    GAP_KDIST_ENCKEY = (1 << 0),
        // Encryption key in distribution
    GAP_KDIST_IDKEY  = (1 << 1),
        // IRK (ID key)in distribution
    GAP_KDIST_SIGNKEY= (1 << 2),
        // CSRK(Signature key) in distribution
    GAP_KDIST_LAST =   (1 << 3),
};


// Security Defines

enum gap_sec_req
{
    GAP_NO_SEC = 0x00,
        // No security (no authentication and encryption)
    GAP_SEC1_NOAUTH_PAIR_ENC,
        // Unauthenticated pairing with encryption
    GAP_SEC1_AUTH_PAIR_ENC,
        // Authenticated pairing with encryption
    GAP_SEC2_NOAUTH_DATA_SGN,
        // Unauthenticated pairing with data signing
    GAP_SEC2_AUTH_DATA_SGN,
        // Authentication pairing with data signing
    GAP_SEC_UNDEFINED
        // Unrecognized security
};


// Authorization setting

enum gap_authz
{
    GAP_AUTHZ_NOT_SET = 0x00,
        // Authorization not set, application informed when authorization requested
    GAP_AUTHZ_ACCEPT  = 0x01,
        // Authorization request automatically accepted
    GAP_AUTHZ_REJECT  = 0x02,
        // Authorization request automatically rejected
};


// AD Type Flag - Bit set 

#define GAP_LE_LIM_DISCOVERABLE_FLG             0x01
    // Limited discovery flag - AD Flag
#define GAP_LE_GEN_DISCOVERABLE_FLG             0x02
    // General discovery flag - AD Flag
#define GAP_BR_EDR_NOT_SUPPORTED                0x04
    // Legacy BT not supported - AD Flag
#define GAP_SIMUL_BR_EDR_LE_CONTROLLER          0x08
    // Dual mode for controller supported (BR/EDR/LE) - AD Flag
#define GAP_SIMUL_BR_EDR_LE_HOST                0x10
    // Dual mode for host supported (BR/EDR/LE) - AD Flag


// Miscellaneous Defines 

#define GAP_INVALID_CONIDX                      0xFF
    // Invalid connection index
#define GAP_INVALID_CONHDL                      0xFFFF
    // Invalid connection handle




#if 0	// PRH OLD

    
    /*
     * Standard profile descriptor identifiers; note these
     * may be identical to some of the service classes defined above
     */
    #define SDP_SERVER_PROFILE_ID		SDP_SERVER_SVCLASS_ID
    #define BROWSE_GRP_DESC_PROFILE_ID	BROWSE_GRP_DESC_SVCLASS_ID
    #define SERIAL_PORT_PROFILE_ID		SERIAL_PORT_SVCLASS_ID
    #define LAN_ACCESS_PROFILE_ID		LAN_ACCESS_SVCLASS_ID
    #define DIALUP_NET_PROFILE_ID		DIALUP_NET_SVCLASS_ID
    #define IRMC_SYNC_PROFILE_ID		IRMC_SYNC_SVCLASS_ID
    #define OBEX_OBJPUSH_PROFILE_ID		OBEX_OBJPUSH_SVCLASS_ID
    #define OBEX_FILETRANS_PROFILE_ID	OBEX_FILETRANS_SVCLASS_ID
    #define IRMC_SYNC_CMD_PROFILE_ID	IRMC_SYNC_CMD_SVCLASS_ID
    #define HEADSET_PROFILE_ID		HEADSET_SVCLASS_ID
    #define CORDLESS_TELEPHONY_PROFILE_ID	CORDLESS_TELEPHONY_SVCLASS_ID
    #define AUDIO_SOURCE_PROFILE_ID		AUDIO_SOURCE_SVCLASS_ID
    #define AUDIO_SINK_PROFILE_ID		AUDIO_SINK_SVCLASS_ID
    #define AV_REMOTE_TARGET_PROFILE_ID	AV_REMOTE_TARGET_SVCLASS_ID
    #define ADVANCED_AUDIO_PROFILE_ID	ADVANCED_AUDIO_SVCLASS_ID
    #define AV_REMOTE_PROFILE_ID		AV_REMOTE_SVCLASS_ID
    #define VIDEO_CONF_PROFILE_ID		VIDEO_CONF_SVCLASS_ID
    #define INTERCOM_PROFILE_ID		INTERCOM_SVCLASS_ID
    #define FAX_PROFILE_ID			FAX_SVCLASS_ID
    #define HEADSET_AGW_PROFILE_ID		HEADSET_AGW_SVCLASS_ID
    #define WAP_PROFILE_ID			WAP_SVCLASS_ID
    #define WAP_CLIENT_PROFILE_ID		WAP_CLIENT_SVCLASS_ID
    #define PANU_PROFILE_ID			PANU_SVCLASS_ID
    #define NAP_PROFILE_ID			NAP_SVCLASS_ID
    #define GN_PROFILE_ID			GN_SVCLASS_ID
    #define DIRECT_PRINTING_PROFILE_ID	DIRECT_PRINTING_SVCLASS_ID
    #define REFERENCE_PRINTING_PROFILE_ID	REFERENCE_PRINTING_SVCLASS_ID
    #define IMAGING_PROFILE_ID		IMAGING_SVCLASS_ID
    #define IMAGING_RESPONDER_PROFILE_ID	IMAGING_RESPONDER_SVCLASS_ID
    #define IMAGING_ARCHIVE_PROFILE_ID	IMAGING_ARCHIVE_SVCLASS_ID
    #define IMAGING_REFOBJS_PROFILE_ID	IMAGING_REFOBJS_SVCLASS_ID
    #define HANDSFREE_PROFILE_ID		HANDSFREE_SVCLASS_ID
    #define HANDSFREE_AGW_PROFILE_ID	HANDSFREE_AGW_SVCLASS_ID
    #define DIRECT_PRT_REFOBJS_PROFILE_ID	DIRECT_PRT_REFOBJS_SVCLASS_ID
    #define REFLECTED_UI_PROFILE_ID		REFLECTED_UI_SVCLASS_ID
    #define BASIC_PRINTING_PROFILE_ID	BASIC_PRINTING_SVCLASS_ID
    #define PRINTING_STATUS_PROFILE_ID	PRINTING_STATUS_SVCLASS_ID
    #define HID_PROFILE_ID			HID_SVCLASS_ID
    #define HCR_PROFILE_ID			HCR_SCAN_SVCLASS_ID
    #define HCR_PRINT_PROFILE_ID		HCR_PRINT_SVCLASS_ID
    #define HCR_SCAN_PROFILE_ID		HCR_SCAN_SVCLASS_ID
    #define CIP_PROFILE_ID			CIP_SVCLASS_ID
    #define VIDEO_CONF_GW_PROFILE_ID	VIDEO_CONF_GW_SVCLASS_ID
    #define UDI_MT_PROFILE_ID		UDI_MT_SVCLASS_ID
    #define UDI_TA_PROFILE_ID		UDI_TA_SVCLASS_ID
    #define AV_PROFILE_ID			AV_SVCLASS_ID
    #define SAP_PROFILE_ID			SAP_SVCLASS_ID
    #define PBAP_PCE_PROFILE_ID		PBAP_PCE_SVCLASS_ID
    #define PBAP_PSE_PROFILE_ID		PBAP_PSE_SVCLASS_ID
    #define PBAP_PROFILE_ID			PBAP_SVCLASS_ID
    #define PNP_INFO_PROFILE_ID		PNP_INFO_SVCLASS_ID
    #define GENERIC_NETWORKING_PROFILE_ID	GENERIC_NETWORKING_SVCLASS_ID
    #define GENERIC_FILETRANS_PROFILE_ID	GENERIC_FILETRANS_SVCLASS_ID
    #define GENERIC_AUDIO_PROFILE_ID	GENERIC_AUDIO_SVCLASS_ID
    #define GENERIC_TELEPHONY_PROFILE_ID	GENERIC_TELEPHONY_SVCLASS_ID
    #define UPNP_PROFILE_ID			UPNP_SVCLASS_ID
    #define UPNP_IP_PROFILE_ID		UPNP_IP_SVCLASS_ID
    #define UPNP_PAN_PROFILE_ID		UPNP_PAN_SVCLASS_ID
    #define UPNP_LAP_PROFILE_ID		UPNP_LAP_SVCLASS_ID
    #define UPNP_L2CAP_PROFILE_ID		UPNP_L2CAP_SVCLASS_ID
    #define VIDEO_SOURCE_PROFILE_ID		VIDEO_SOURCE_SVCLASS_ID
    #define VIDEO_SINK_PROFILE_ID		VIDEO_SINK_SVCLASS_ID
    #define VIDEO_DISTRIBUTION_PROFILE_ID	VIDEO_DISTRIBUTION_SVCLASS_ID
    #define HDP_PROFILE_ID			HDP_SVCLASS_ID
    #define HDP_SOURCE_PROFILE_ID		HDP_SOURCE_SVCLASS_ID
    #define HDP_SINK_PROFILE_ID		HDP_SINK_SVCLASS_ID
    #define APPLE_AGENT_PROFILE_ID		APPLE_AGENT_SVCLASS_ID
    #define GENERIC_ACCESS_PROFILE_ID	0x1800
    #define GENERIC_ATTRIB_PROFILE_ID	GENERIC_ATTRIB_SVCLASS_ID
    
    /*
     * Compatibility macros for the old MDP acronym
     */
    #define MDP_SVCLASS_ID			HDP_SVCLASS_ID
    #define MDP_SOURCE_SVCLASS_ID		HDP_SOURCE_SVCLASS_ID
    #define MDP_SINK_SVCLASS_ID		HDP_SINK_SVCLASS_ID
    #define MDP_PROFILE_ID			HDP_PROFILE_ID
    #define MDP_SOURCE_PROFILE_ID		HDP_SOURCE_PROFILE_ID
    #define MDP_SINK_PROFILE_ID		HDP_SINK_PROFILE_ID
    
    /*
     * Attribute identifier codes
     */
    #define SDP_SERVER_RECORD_HANDLE		0x0000
    
    /*
     * Possible values for attribute-id are listed below.
     * See SDP Spec, section "Service Attribute Definitions" for more details.
     */
    #define SDP_ATTR_RECORD_HANDLE			0x0000
    #define SDP_ATTR_SVCLASS_ID_LIST		0x0001
    #define SDP_ATTR_RECORD_STATE			0x0002
    #define SDP_ATTR_SERVICE_ID			0x0003
    #define SDP_ATTR_PROTO_DESC_LIST		0x0004
    #define SDP_ATTR_BROWSE_GRP_LIST		0x0005
    #define SDP_ATTR_LANG_BASE_ATTR_ID_LIST		0x0006
    #define SDP_ATTR_SVCINFO_TTL			0x0007
    #define SDP_ATTR_SERVICE_AVAILABILITY		0x0008
    #define SDP_ATTR_PFILE_DESC_LIST		0x0009
    #define SDP_ATTR_DOC_URL			0x000a
    #define SDP_ATTR_CLNT_EXEC_URL			0x000b
    #define SDP_ATTR_ICON_URL			0x000c
    #define SDP_ATTR_ADD_PROTO_DESC_LIST		0x000d
    
    #define SDP_ATTR_GROUP_ID			0x0200
    #define SDP_ATTR_IP_SUBNET			0x0200
    #define SDP_ATTR_VERSION_NUM_LIST		0x0200
    #define SDP_ATTR_SUPPORTED_FEATURES_LIST	0x0200
    #define SDP_ATTR_GOEP_L2CAP_PSM			0x0200
    #define SDP_ATTR_SVCDB_STATE			0x0201
    
    #define SDP_ATTR_SERVICE_VERSION		0x0300
    #define SDP_ATTR_EXTERNAL_NETWORK		0x0301
    #define SDP_ATTR_SUPPORTED_DATA_STORES_LIST	0x0301
    #define SDP_ATTR_DATA_EXCHANGE_SPEC		0x0301
    #define SDP_ATTR_NETWORK			0x0301
    #define SDP_ATTR_FAX_CLASS1_SUPPORT		0x0302
    #define SDP_ATTR_REMOTE_AUDIO_VOLUME_CONTROL	0x0302
    #define SDP_ATTR_MCAP_SUPPORTED_PROCEDURES	0x0302
    #define SDP_ATTR_FAX_CLASS20_SUPPORT		0x0303
    #define SDP_ATTR_SUPPORTED_FORMATS_LIST		0x0303
    #define SDP_ATTR_FAX_CLASS2_SUPPORT		0x0304
    #define SDP_ATTR_AUDIO_FEEDBACK_SUPPORT		0x0305
    #define SDP_ATTR_NETWORK_ADDRESS		0x0306
    #define SDP_ATTR_WAP_GATEWAY			0x0307
    #define SDP_ATTR_HOMEPAGE_URL			0x0308
    #define SDP_ATTR_WAP_STACK_TYPE			0x0309
    #define SDP_ATTR_SECURITY_DESC			0x030a
    #define SDP_ATTR_NET_ACCESS_TYPE		0x030b
    #define SDP_ATTR_MAX_NET_ACCESSRATE		0x030c
    #define SDP_ATTR_IP4_SUBNET			0x030d
    #define SDP_ATTR_IP6_SUBNET			0x030e
    #define SDP_ATTR_SUPPORTED_CAPABILITIES		0x0310
    #define SDP_ATTR_SUPPORTED_FEATURES		0x0311
    #define SDP_ATTR_SUPPORTED_FUNCTIONS		0x0312
    #define SDP_ATTR_TOTAL_IMAGING_DATA_CAPACITY	0x0313
    #define SDP_ATTR_SUPPORTED_REPOSITORIES		0x0314
    #define SDP_ATTR_MAS_INSTANCE_ID		0x0315
    #define SDP_ATTR_SUPPORTED_MESSAGE_TYPES	0x0316
    
    #define SDP_ATTR_SPECIFICATION_ID		0x0200
    #define SDP_ATTR_VENDOR_ID			0x0201
    #define SDP_ATTR_PRODUCT_ID			0x0202
    #define SDP_ATTR_VERSION			0x0203
    #define SDP_ATTR_PRIMARY_RECORD			0x0204
    #define SDP_ATTR_VENDOR_ID_SOURCE		0x0205
    
    #define SDP_ATTR_HID_DEVICE_RELEASE_NUMBER	0x0200
    #define SDP_ATTR_HID_PARSER_VERSION		0x0201
    #define SDP_ATTR_HID_DEVICE_SUBCLASS		0x0202
    #define SDP_ATTR_HID_COUNTRY_CODE		0x0203
    #define SDP_ATTR_HID_VIRTUAL_CABLE		0x0204
    #define SDP_ATTR_HID_RECONNECT_INITIATE		0x0205
    #define SDP_ATTR_HID_DESCRIPTOR_LIST		0x0206
    #define SDP_ATTR_HID_LANG_ID_BASE_LIST		0x0207
    #define SDP_ATTR_HID_SDP_DISABLE		0x0208
    #define SDP_ATTR_HID_BATTERY_POWER		0x0209
    #define SDP_ATTR_HID_REMOTE_WAKEUP		0x020a
    #define SDP_ATTR_HID_PROFILE_VERSION		0x020b
    #define SDP_ATTR_HID_SUPERVISION_TIMEOUT	0x020c
    #define SDP_ATTR_HID_NORMALLY_CONNECTABLE	0x020d
    #define SDP_ATTR_HID_BOOT_DEVICE		0x020e
    
    /*
     * These identifiers are based on the SDP spec stating that
     * "base attribute id of the primary (universal) language must be 0x0100"
     *
     * Other languages should have their own offset; e.g.:
     * #define XXXLangBase yyyy
     * #define AttrServiceName_XXX	0x0000+XXXLangBase
     */
    #define SDP_PRIMARY_LANG_BASE		0x0100
    
    #define SDP_ATTR_SVCNAME_PRIMARY	0x0000 + SDP_PRIMARY_LANG_BASE
    #define SDP_ATTR_SVCDESC_PRIMARY	0x0001 + SDP_PRIMARY_LANG_BASE
    #define SDP_ATTR_PROVNAME_PRIMARY	0x0002 + SDP_PRIMARY_LANG_BASE
    
    /*
     * The Data representation in SDP PDUs (pps 339, 340 of BT SDP Spec)
     * These are the exact data type+size descriptor values
     * that go into the PDU buffer.
     *
     * The datatype (leading 5bits) + size descriptor (last 3 bits)
     * is 8 bits. The size descriptor is critical to extract the
     * right number of bytes for the data value from the PDU.
     *
     * For most basic types, the datatype+size descriptor is
     * straightforward. However for constructed types and strings,
     * the size of the data is in the next "n" bytes following the
     * 8 bits (datatype+size) descriptor. Exactly what the "n" is
     * specified in the 3 bits of the data size descriptor.
     *
     * TextString and URLString can be of size 2^{8, 16, 32} bytes
     * DataSequence and DataSequenceAlternates can be of size 2^{8, 16, 32}
     * The size are computed post-facto in the API and are not known apriori
     */
    #define SDP_DATA_NIL		0x00
    #define SDP_UINT8		0x08
    #define SDP_UINT16		0x09
    #define SDP_UINT32		0x0A
    #define SDP_UINT64		0x0B
    #define SDP_UINT128		0x0C
    #define SDP_INT8		0x10
    #define SDP_INT16		0x11
    #define SDP_INT32		0x12
    #define SDP_INT64		0x13
    #define SDP_INT128		0x14
    #define SDP_UUID_UNSPEC		0x18
    #define SDP_UUID16		0x19
    #define SDP_UUID32		0x1A
    #define SDP_UUID128		0x1C
    #define SDP_TEXT_STR_UNSPEC	0x20
    #define SDP_TEXT_STR8		0x25
    #define SDP_TEXT_STR16		0x26
    #define SDP_TEXT_STR32		0x27
    #define SDP_BOOL		0x28
    #define SDP_SEQ_UNSPEC		0x30
    #define SDP_SEQ8		0x35
    #define SDP_SEQ16		0x36
    #define SDP_SEQ32		0x37
    #define SDP_ALT_UNSPEC		0x38
    #define SDP_ALT8		0x3D
    #define SDP_ALT16		0x3E
    #define SDP_ALT32		0x3F
    #define SDP_URL_STR_UNSPEC	0x40
    #define SDP_URL_STR8		0x45
    #define SDP_URL_STR16		0x46
    #define SDP_URL_STR32		0x47
    
    /*
     * The PDU identifiers of SDP packets between client and server
     */
    /*
     * Some additions to support service registration.
     * These are outside the scope of the Bluetooth specification
     */
    #define SDP_SVC_REGISTER_REQ	0x75
    #define SDP_SVC_REGISTER_RSP	0x76
    #define SDP_SVC_UPDATE_REQ	0x77
    #define SDP_SVC_UPDATE_RSP	0x78
    #define SDP_SVC_REMOVE_REQ	0x79
    #define SDP_SVC_REMOVE_RSP	0x80
    
    /*
     * SDP Error codes
     */
    #define SDP_INVALID_VERSION		0x0001
    #define SDP_INVALID_RECORD_HANDLE	0x0002
    #define SDP_INVALID_SYNTAX		0x0003
    #define SDP_INVALID_PDU_SIZE		0x0004
    #define SDP_INVALID_CSTATE		0x0005
    
    
    //------------------------------------------
    // RFCOMM FROM HERE DOWN	
    //------------------------------------------
    
    #define RFCOMM_CONN_TIMEOUT (HZ * 30)
    #define RFCOMM_DISC_TIMEOUT (HZ * 20)
    #define RFCOMM_AUTH_TIMEOUT (HZ * 25)
    #define RFCOMM_IDLE_TIMEOUT (HZ * 2)
    
    #define RFCOMM_DEFAULT_MTU	127
    #define RFCOMM_DEFAULT_CREDITS	7
    
    #define RFCOMM_MAX_L2CAP_MTU	1013
    #define RFCOMM_MAX_CREDITS	40
    
    #define RFCOMM_SKB_HEAD_RESERVE	8
    #define RFCOMM_SKB_TAIL_RESERVE	2
    #define RFCOMM_SKB_RESERVE  (RFCOMM_SKB_HEAD_RESERVE + RFCOMM_SKB_TAIL_RESERVE)
    
    #define RFCOMM_SABM	0x2f
    #define RFCOMM_DISC	0x43
    #define RFCOMM_UA	0x63
    #define RFCOMM_DM	0x0f
    #define RFCOMM_UIH	0xef
    
    #define RFCOMM_TEST	0x08
    #define RFCOMM_FCON	0x28
    #define RFCOMM_FCOFF	0x18
    #define RFCOMM_MSC	0x38
    #define RFCOMM_RPN	0x24
    #define RFCOMM_RLS	0x14
    #define RFCOMM_PN	0x20
    #define RFCOMM_NSC	0x04
    
    #define RFCOMM_V24_FC	0x02
    #define RFCOMM_V24_RTC	0x04
    #define RFCOMM_V24_RTR	0x08
    #define RFCOMM_V24_IC	0x40
    #define RFCOMM_V24_DV	0x80
    
    #define RFCOMM_RPN_BR_2400	0x0
    #define RFCOMM_RPN_BR_4800	0x1
    #define RFCOMM_RPN_BR_7200	0x2
    #define RFCOMM_RPN_BR_9600	0x3
    #define RFCOMM_RPN_BR_19200	0x4
    #define RFCOMM_RPN_BR_38400	0x5
    #define RFCOMM_RPN_BR_57600	0x6
    #define RFCOMM_RPN_BR_115200	0x7
    #define RFCOMM_RPN_BR_230400	0x8
    
    #define RFCOMM_RPN_DATA_5	0x0
    #define RFCOMM_RPN_DATA_6	0x1
    #define RFCOMM_RPN_DATA_7	0x2
    #define RFCOMM_RPN_DATA_8	0x3
    
    #define RFCOMM_RPN_STOP_1	0
    #define RFCOMM_RPN_STOP_15	1
    
    #define RFCOMM_RPN_PARITY_NONE	0x0
    #define RFCOMM_RPN_PARITY_ODD	0x1
    #define RFCOMM_RPN_PARITY_EVEN	0x3
    #define RFCOMM_RPN_PARITY_MARK	0x5
    #define RFCOMM_RPN_PARITY_SPACE	0x7
    
    #define RFCOMM_RPN_FLOW_NONE	0x00
    
    #define RFCOMM_RPN_XON_CHAR	0x11
    #define RFCOMM_RPN_XOFF_CHAR	0x13
    
    #define RFCOMM_RPN_PM_BITRATE		0x0001
    #define RFCOMM_RPN_PM_DATA		0x0002
    #define RFCOMM_RPN_PM_STOP		0x0004
    #define RFCOMM_RPN_PM_PARITY		0x0008
    #define RFCOMM_RPN_PM_PARITY_TYPE	0x0010
    #define RFCOMM_RPN_PM_XON		0x0020
    #define RFCOMM_RPN_PM_XOFF		0x0040
    #define RFCOMM_RPN_PM_FLOW		0x3F00
    
    #define RFCOMM_RPN_PM_ALL		0x3F7F

	typedef u16 	__le16;
	typedef u8 bdaddr_t[6];
		// these types added by me to support
		// following strutures IF i use them
	
	struct rfcomm_hdr {
		u8 addr;
		u8 ctrl;
		u8 len;    // Actual size can be 2 bytes
	} __attribute__ ((packed));
	
	struct rfcomm_cmd {
		u8 addr;
		u8 ctrl;
		u8 len;
		u8 fcs;
	} __attribute__ ((packed));
	
	struct rfcomm_mcc {
		u8 type;
		u8 len;
	} __attribute__ ((packed));
	
	struct rfcomm_pn {
		u8  dlci;
		u8  flow_ctrl;
		u8  priority;
		u8  ack_timer;
		__le16 mtu;
		u8  max_retrans;
		u8  credits;
	} __attribute__ ((packed));
	
	struct rfcomm_rpn {
		u8  dlci;
		u8  bit_rate;
		u8  line_settings;
		u8  flow_ctrl;
		u8  xon_char;
		u8  xoff_char;
		__le16 param_mask;
	} __attribute__ ((packed));
	
	struct rfcomm_rls {
		u8  dlci;
		u8  status;
	} __attribute__ ((packed));
	
	struct rfcomm_msc {
		u8  dlci;
		u8  v24_sig;
	} __attribute__ ((packed));
	
	
	/* ---- Core structures, flags etc ---- */

	struct rfcomm_session {
		struct list_head list;
		struct socket   *sock;
		struct timer_list timer;
		unsigned long    state;
		unsigned long    flags;
		atomic_t         refcnt;
		int              initiator;
	
		/* Default DLC parameters */
		int    cfc;
		uint   mtu;
	
		struct list_head dlcs;
	};
	
	struct rfcomm_dlc {
		struct list_head      list;
		struct rfcomm_session *session;
		struct sk_buff_head   tx_queue;
		struct timer_list     timer;
	
		spinlock_t    lock;
		unsigned long state;
		unsigned long flags;
		atomic_t      refcnt;
		u8            dlci;
		u8            addr;
		u8            priority;
		u8            v24_sig;
		u8            remote_v24_sig;
		u8            mscex;
		u8            out;
		u8            sec_level;
		u8            role_switch;
		u32           defer_setup;
	
		uint          mtu;
		uint          cfc;
		uint          rx_credits;
		uint          tx_credits;
	
		void          *owner;
	
		void (*data_ready)(struct rfcomm_dlc *d, struct sk_buff *skb);
		void (*state_change)(struct rfcomm_dlc *d, int err);
		void (*modem_status)(struct rfcomm_dlc *d, u8 v24_sig);
	};
	

    
    /* DLC and session flags */
    #define RFCOMM_RX_THROTTLED 0
    #define RFCOMM_TX_THROTTLED 1
    #define RFCOMM_TIMED_OUT    2
    #define RFCOMM_MSC_PENDING  3
    #define RFCOMM_SEC_PENDING  4
    #define RFCOMM_AUTH_PENDING 5
    #define RFCOMM_AUTH_ACCEPT  6
    #define RFCOMM_AUTH_REJECT  7
    #define RFCOMM_DEFER_SETUP  8
    
    /* Scheduling flags and events */
    #define RFCOMM_SCHED_STATE  0
    #define RFCOMM_SCHED_RX     1
    #define RFCOMM_SCHED_TX     2
    #define RFCOMM_SCHED_TIMEO  3
    #define RFCOMM_SCHED_AUTH   4
    #define RFCOMM_SCHED_WAKEUP 31
    
    /* MSC exchange flags */
    #define RFCOMM_MSCEX_TX     1
    #define RFCOMM_MSCEX_RX     2
    #define RFCOMM_MSCEX_OK     (RFCOMM_MSCEX_TX + RFCOMM_MSCEX_RX)
    
    /* CFC states */
    #define RFCOMM_CFC_UNKNOWN  -1
    #define RFCOMM_CFC_DISABLED 0
    #define RFCOMM_CFC_ENABLED  RFCOMM_MAX_CREDITS



	/* ---- RFCOMM SEND RPN ---- */
	int rfcomm_send_rpn(struct rfcomm_session *s, int cr, u8 dlci,
				u8 bit_rate, u8 data_bits, u8 stop_bits,
				u8 parity, u8 flow_ctrl_settings, 
				u8 xon_char, u8 xoff_char, u16 param_mask);
	
	/* ---- RFCOMM DLCs (channels) ---- */
	
	struct rfcomm_dlc *rfcomm_dlc_alloc(gfp_t prio);
	void rfcomm_dlc_free(struct rfcomm_dlc *d);
	int  rfcomm_dlc_open(struct rfcomm_dlc *d, bdaddr_t *src, bdaddr_t *dst, u8 channel);
	int  rfcomm_dlc_close(struct rfcomm_dlc *d, int reason);
	int  rfcomm_dlc_send(struct rfcomm_dlc *d, struct sk_buff *skb);
	int  rfcomm_dlc_set_modem_status(struct rfcomm_dlc *d, u8 v24_sig);
	int  rfcomm_dlc_get_modem_status(struct rfcomm_dlc *d, u8 *v24_sig);
	void rfcomm_dlc_accept(struct rfcomm_dlc *d);
	
	#define rfcomm_dlc_lock(d)     spin_lock(&d->lock)
	#define rfcomm_dlc_unlock(d)   spin_unlock(&d->lock)
	
	static inline void rfcomm_dlc_hold(struct rfcomm_dlc *d)
	{
		atomic_inc(&d->refcnt);
	}
	
	static inline void rfcomm_dlc_put(struct rfcomm_dlc *d)
	{
		if (atomic_dec_and_test(&d->refcnt))
			rfcomm_dlc_free(d);
	}
	
	extern void __rfcomm_dlc_throttle(struct rfcomm_dlc *d);
	extern void __rfcomm_dlc_unthrottle(struct rfcomm_dlc *d);
	
	static inline void rfcomm_dlc_throttle(struct rfcomm_dlc *d)
	{
		if (!test_and_set_bit(RFCOMM_RX_THROTTLED, &d->flags))
			__rfcomm_dlc_throttle(d);
	}
	
	static inline void rfcomm_dlc_unthrottle(struct rfcomm_dlc *d)
	{
		if (test_and_clear_bit(RFCOMM_RX_THROTTLED, &d->flags))
			__rfcomm_dlc_unthrottle(d);
	}
	
	/* ---- RFCOMM sessions ---- */
	void   rfcomm_session_getaddr(struct rfcomm_session *s, bdaddr_t *src, bdaddr_t *dst);
	
	static inline void rfcomm_session_hold(struct rfcomm_session *s)
	{
		atomic_inc(&s->refcnt);
	}
	
	/* ---- RFCOMM sockets ---- */
	struct sockaddr_rc {
		sa_family_t	rc_family;
		bdaddr_t	rc_bdaddr;
		u8		rc_channel;
	};
	
	#define RFCOMM_CONNINFO	0x02
	struct rfcomm_conninfo {
		__u16 hci_handle;
		__u8  dev_class[3];
	};
	
	
	#define RFCOMM_LM	0x03
	#define RFCOMM_LM_MASTER	0x0001
	#define RFCOMM_LM_AUTH		0x0002
	#define RFCOMM_LM_ENCRYPT	0x0004
	#define RFCOMM_LM_TRUSTED	0x0008
	#define RFCOMM_LM_RELIABLE	0x0010
	#define RFCOMM_LM_SECURE	0x0020
	
	#define rfcomm_pi(sk) ((struct rfcomm_pinfo *) sk)

	struct rfcomm_pinfo {
		struct bt_sock bt;
		struct rfcomm_dlc   *dlc;
		u8     channel;
		u8     sec_level;
		u8     role_switch;
	};

	int  rfcomm_init_sockets(void);
	void rfcomm_cleanup_sockets(void);
	int  rfcomm_connect_ind(struct rfcomm_session *s, u8 channel, struct rfcomm_dlc **d);

	/* ---- RFCOMM TTY ---- */
	#define RFCOMM_MAX_DEV  256
	
	#define RFCOMMCREATEDEV		_IOW('R', 200, int)
	#define RFCOMMRELEASEDEV	_IOW('R', 201, int)
	#define RFCOMMGETDEVLIST	_IOR('R', 210, int)
	#define RFCOMMGETDEVINFO	_IOR('R', 211, int)
	#define RFCOMMSTEALDLC		_IOW('R', 220, int)
	
	#define RFCOMM_REUSE_DLC      0
	#define RFCOMM_RELEASE_ONHUP  1
	#define RFCOMM_HANGUP_NOW     2
	#define RFCOMM_TTY_ATTACHED   3
	#define RFCOMM_TTY_RELEASED   4
	
	struct rfcomm_dev_req {
		s16      dev_id;
		u32      flags;
		bdaddr_t src;
		bdaddr_t dst;
		u8       channel;
	};
	
	struct rfcomm_dev_info {
		s16      id;
		u32      flags;
		u16      state;
		bdaddr_t src;
		bdaddr_t dst;
		u8       channel;
	};
	
	struct rfcomm_dev_list_req {
		u16      dev_num;
		struct   rfcomm_dev_info dev_info[0];
	};


	int  rfcomm_dev_ioctl(struct sock *sk, unsigned int cmd, void __user *arg);
	
	#ifdef CONFIG_BT_RFCOMM_TTY
		int  rfcomm_init_ttys(void);
		void rfcomm_cleanup_ttys(void);
	#else
		static inline int rfcomm_init_ttys(void)
		{
			return 0;
		}
		static inline void rfcomm_cleanup_ttys(void)
		{
		}
	#endif
	
#endif // prh OLD


#endif // _GAP_DEFS_H_
