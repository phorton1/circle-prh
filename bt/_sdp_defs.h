
#ifndef __SDP_DEFS_H
#define __SDP_DEFS_H

// SDP PDU command types

#define SDP_COMMAND_ERROR_RESPONSE		    0x01
#define SDP_COMMAND_SEARCH_REQUEST	        0x02
#define SDP_COMMAND_SEARCH_RESPONSE	        0x03
#define SDP_COMMAND_ATTR_REQUEST	        0x04
#define SDP_COMMAND_ATTR_RESPONSE	        0x05
#define SDP_COMMAND_SEARCH_ATTR_REQUEST	    0x06
#define SDP_COMMAND_SEARCH_ATTR_RESPONSE	0x07
    
// Protocol UUIDs
 
#define UUID_PROTO_SDP	                    0x0001
#define UUID_PROTO_UDP	                    0x0002
#define UUID_PROTO_RFCOMM	                0x0003
#define UUID_PROTO_TCP	                    0x0004
#define UUID_PROTO_TCS_BIN	                0x0005
#define UUID_PROTO_TCS_AT	                0x0006
#define UUID_PROTO_ATT	                    0x0007
#define UUID_PROTO_OBEX	                    0x0008
#define UUID_PROTO_IP		                0x0009
#define UUID_PROTO_FTP	                    0x000a
#define UUID_PROTO_HTTP	                    0x000c
#define UUID_PROTO_WSP	                    0x000e
#define UUID_PROTO_BNEP	                    0x000f      // PAN
#define UUID_PROTO_UPNP	                    0x0010
#define UUID_PROTO_HIDP	                    0x0011      // HID 
#define UUID_PROTO_HCRP_CTRL	            0x0012      // HCRP 
#define UUID_PROTO_HCRP_DATA	            0x0014      // HCRP 
#define UUID_PROTO_HCRP_NOTE	            0x0016      // HCRP 
#define UUID_PROTO_AVCTP	                0x0017      // AVCTP 
#define UUID_PROTO_AVDTP	                0x0019      // AVDTP 
#define UUID_PROTO_CMTP	                    0x001b      // CIP 
#define UUID_PROTO_UDI	                    0x001d      // UDI 
#define UUID_PROTO_MCAP_CTRL	            0x001e
#define UUID_PROTO_MCAP_DATA	            0x001f
#define UUID_PROTO_L2CAP	                0x0100


// Service Class Identifiers

#define UUID_SERVICE_SDP_SERVER		            0x1000
#define UUID_SERVICE_BROWSE_GROUP	            0x1001
#define UUID_SERVICE_PUBLIC_BROWSE_GROUP	    0x1002
#define UUID_SERVICE_SERIAL_PORT		        0x1101
#define UUID_SERVICE_LAN_ACCESS		            0x1102
#define UUID_SERVICE_DIALUP_NET		            0x1103
#define UUID_SERVICE_IRMC_SYNC		            0x1104
#define UUID_SERVICE_OBEX_OBJ_PUSH		        0x1105
#define UUID_SERVICE_OBEX_FILE_TRANSFER         0x1106
#define UUID_SERVICE_IRMC_SYNC_CMD	            0x1107
#define UUID_SERVICE_HEADSET		            0x1108
#define UUID_SERVICE_CORDLESS_TELEPHONY	        0x1109
#define UUID_SERVICE_AUDIO_SOURCE		        0x110a      // A2DP 
#define UUID_SERVICE_AUDIO_SINK		            0x110b      // A2DP 
#define UUID_SERVICE_AV_REMOTE_TARGET	        0x110c      // AVRCP
#define UUID_SERVICE_ADVANCED_AUDIO	            0x110d      // A2DP 
#define UUID_SERVICE_AV_REMOTE		            0x110e      // AVRCP
#define UUID_SERVICE_VIDEO_CONFERENCING	        0x110f      // VCP 
#define UUID_SERVICE_INTERCOM		            0x1110
#define UUID_SERVICE_FAX			            0x1111
#define UUID_SERVICE_HEADSET_AUDIO_GATEWAY      0x1112
#define UUID_SERVICE_WAP			            0x1113
#define UUID_SERVICE_WAP_CLIENT		            0x1114
#define UUID_SERVICE_PANU			            0x1115      // PAN
#define UUID_SERVICE_NAP			            0x1116      // PAN
#define UUID_SERVICE_GN			                0x1117      // PAN
#define UUID_SERVICE_DIRECT_PRINTING	        0x1118      // BPP
#define UUID_SERVICE_REFERENCE_PRINTING	        0x1119      // BPP
#define UUID_SERVICE_IMAGING		            0x111a      // BIP
#define UUID_SERVICE_IMAGING_RESPONDER	        0x111b      // BIP
#define UUID_SERVICE_IMAGING_ARCHIVE	        0x111c      // BIP
#define UUID_SERVICE_IMAGING_REF_OBJS	        0x111d      // BIP
#define UUID_SERVICE_HANDSFREE		            0x111e
#define UUID_SERVICE_HANDSFREE_AUDIO_GATEWAY	0x111f      
#define UUID_SERVICE_DIRECT_PRINTING_REF_OBJS   0x1120      // BPP 
#define UUID_SERVICE_REFLECTED_UI		        0x1121      // BPP 
#define UUID_SERVICE_BASIC_PRINTING	            0x1122      // BPP 
#define UUID_SERVICE_PRINTING_STATUS	        0x1123      // BPP 
#define UUID_SERVICE_HID			            0x1124      // HID 
#define UUID_SERVICE_HARDCOPY_CABLE_REPLACE	    0x1125      // HCRP
#define UUID_SERVICE_HCR_PRINT		            0x1126      // HCRP
#define UUID_SERVICE_HCR_SCAN		            0x1127      // HCRP
#define UUID_SERVICE_COMMON_ISDN_ACCESS         0x1128      // CIP 
#define UUID_SERVICE_VIDEO_CONFERENCING_GW      0x1129      // VCP 
#define UUID_SERVICE_UDI_MT		                0x112a      // UDI 
#define UUID_SERVICE_UDI_TA		                0x112b      // UDI 
#define UUID_SERVICE_AUDIO_VIDEO                0x112c      // VCP 
#define UUID_SERVICE_SIM_ACCESS		            0x112d      // SAP 
#define UUID_SERVICE_PHONEBOOK_ACCESS_PCE	    0x112e      // PBAP
#define UUID_SERVICE_PHONEBOOK_ACCESS_PSE	    0x112f      // PBAP
#define UUID_SERVICE_PHONEBOOK_ACCESS    	    0x1130      // PBAP
#define UUID_SERVICE_MAP_MSE		            0x1132
#define UUID_SERVICE_MAP_MCE		            0x1133
#define UUID_SERVICE_MAP			            0x1134
#define UUID_SERVICE_GNSS			            0x1135
#define UUID_SERVICE_GNSS_SERVER		        0x1136
#define UUID_SERVICE_PNP_INFO		            0x1200
#define UUID_SERVICE_GENERIC_NETWORKING	        0x1201
#define UUID_SERVICE_GENERIC_FILE_TRANSGRT      0x1202
#define UUID_SERVICE_GENERIC_AUDIO	            0x1203
#define UUID_SERVICE_GENERIC_TELEPHONY	        0x1204
#define UUID_SERVICE_UPNP			            0x1205      // ESDP
#define UUID_SERVICE_UPNP_IP		            0x1206      // ESDP
#define UUID_SERVICE_UPNP_PAN		            0x1300      // ESDP
#define UUID_SERVICE_UPNP_LAP		            0x1301      // ESDP
#define UUID_SERVICE_UPNP_L2CAP		            0x1302      // ESDP
#define UUID_SERVICE_VIDEO_SOURCE		        0x1303      // VDP 
#define UUID_SERVICE_VIDEO_SINK		            0x1304      // VDP 
#define UUID_SERVICE_VIDEO_DISTRIBUTION	        0x1305      // VDP 
#define UUID_SERVICE_HDP			            0x1400
#define UUID_SERVICE_HDP_SOURCE		            0x1401
#define UUID_SERVICE_HDP_SINK		            0x1402
#define UUID_SERVICE_APPLE_AGENT		        0x2112

// Bluetooth assigned numbers for Attribute IDs 

#define ATTR_ID_SERVICE_RECORD_HANDLE              0x0000
#define ATTR_ID_SERVICE_CLASS_ID_LIST              0x0001
#define ATTR_ID_SERVICE_RECORD_STATE               0x0002
#define ATTR_ID_SERVICE_SERVICE_ID                 0x0003
#define ATTR_ID_PROTOCOL_DESCRIPTOR_LIST           0x0004
#define ATTR_ID_BROWSE_GROUP_LIST                  0x0005
#define ATTR_ID_LANGUAGE_BASE_ATTRIBUTE_ID_LIST    0x0006
#define ATTR_ID_SERVICE_INFO_TIME_TO_LIVE          0x0007
#define ATTR_ID_SERVICE_AVAILABILITY               0x0008
#define ATTR_ID_BLUETOOTH_PROFILE_DESCRIPTOR_LIST  0x0009
#define ATTR_ID_DOCUMENTATION_URL                  0x000A
#define ATTR_ID_CLIENT_EXECUTABLE_URL              0x000B
#define ATTR_ID_ICON_URL                           0x000C
#define ATTR_ID_ADDITIONAL_PROTOCOL_DESC_LISTS     0x000D
#define ATTR_ID_SERVICE_NAME                       0x0100
#define ATTR_ID_SERVICE_DESCRIPTION                0x0101
#define ATTR_ID_PROVIDER_NAME                      0x0102
#define ATTR_ID_VERSION_NUMBER_LIST                0x0200
#define ATTR_ID_GROUP_ID                           0x0200
#define ATTR_ID_SERVICE_DATABASE_STATE             0x0201
#define ATTR_ID_SERVICE_VERSION                    0x0300
#define ATTR_ID_EXTERNAL_NETWORK                   0x0301   // Cordless Telephony 
#define ATTR_ID_SUPPORTED_DATA_STORES_LIST         0x0301   // Synchronization 
#define ATTR_ID_REMOTE_AUDIO_VOLUME_CONTROL        0x0302   // GAP 
#define ATTR_ID_SUPPORTED_FORMATS_LIST             0x0303   // OBEX Object Push 
#define ATTR_ID_FAX_CLASS_1_SUPPORT                0x0302   // Fax 
#define ATTR_ID_FAX_CLASS_2_0_SUPPORT              0x0303
#define ATTR_ID_FAX_CLASS_2_SUPPORT                0x0304
#define ATTR_ID_AUDIO_FEEDBACK_SUPPORT             0x0305
#define ATTR_ID_SECURITY_DESCRIPTION               0x030a   // PAN 
#define ATTR_ID_NET_ACCESS_TYPE                    0x030b   // PAN 
#define ATTR_ID_MAX_NET_ACCESS_RATE                0x030c   // PAN 
#define ATTR_ID_IPV4_SUBNET                        0x030d   // PAN 
#define ATTR_ID_IPV6_SUBNET                        0x030e   // PAN 
#define ATTR_ID_SUPPORTED_CAPABILITIES             0x0310   // Imaging 
#define ATTR_ID_SUPPORTED_FEATURES                 0x0311   // Imaging and Handsfree 
#define ATTR_ID_SUPPORTED_FUNCTIONS                0x0312   // Imaging 
#define ATTR_ID_TOTAL_IMAGING_DATA_CAPACITY        0x0313   // Imaging 
#define ATTR_ID_SUPPORTED_REPOSITORIES             0x0314   // PBAP 

// #define GENERIC_ATTRIB	        0x1801

#endif 
