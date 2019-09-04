#include <circle/types.h>
#include <circle/util.h>
#include <circle/alloc.h>
#include <circle/string.h>
#include "_sdp_defs.h"
#include "sdpLayer.h"

#define SHOW_DE_TYPE   1

  
#define EIO  1
#define ENOMEM  10

#define NULL  0


u16 be16(u16 v)     { return ((v >> 8) & 0xff) + ((v << 8) & 0xff00); }

bool isPrint(u8 c)  { return c >= ' '; }

void printRaw(int level, sdpRequest *req)
{
    printf("frame(%d/%d) len=%d\n",req->parse_frame_num,req->num_frames,req->parse_frame_len);
    display_bytes("printRaw",req->parse_ptr,req->parse_frame_len);
}

void printIndent(int level)
{
    while (level > 0)
    {
        printf("    ");
        level--;
    }
}


// parser primitives

void inc_parse(sdpRequest *req, int amt)
{
    req->parse_frame_len -= amt;
    assert(req->parse_frame_len >= 0);

    if (req->parse_frame_len <= 0)
    {
        u8 fnum = req->parse_frame_num++;
        if (fnum < req->num_frames)
        {
            req->parse_ptr = req->frame[fnum].data;
            req->parse_frame_len = req->frame[fnum].len;
        }
        else
        {
            req->parse_ptr = 0;
            req->parse_frame_len = 0;
        }
    }
    else
    {
        req->parse_ptr += amt;
    }
    
    req->bytes_parsed += amt;
}


u8 get_u8(sdpRequest *req)
{
	u8 *u8_ptr = req->parse_ptr;
    assert(u8_ptr);
    u8 val = *u8_ptr & 0xff;
    inc_parse(req,1);
	return val;
}
u16 get_u16(sdpRequest *req)
{
	u16 val = 
        (get_u8(req) <<  8) + 
        (get_u8(req) <<  0);
	return val;
}
u32 get_u32(sdpRequest *req)
{
	u32 val =
        (get_u8(req) << 24) + 
        (get_u8(req) << 16) + 
        (get_u8(req) <<  8) + 
        (get_u8(req) <<  0);
	return val;
}
u64 get_u64(sdpRequest *req)
{
	u64 val =
        get_u32(req) * 0x10000 +
        get_u32(req);
    return val;    
}
void get_u128(sdpRequest *req, u64 *l, u64 *h)
{
	*h = get_u64(req);
	*l = get_u64(req);
}




//--------------------------------------
// DE's 
//--------------------------------------
// DE Type Descriptor

#define DE_NULL   0
#define DE_UINT   1
#define DE_INT    2
#define DE_UUID   3
#define DE_STRING 4
#define DE_BOOL   5
#define DE_SEQ    6
#define DE_ALT    7
#define DE_URL    8

// DE Size Lookup Table

struct de_size_table_t
{
	int addl_bits;
	int num_bytes;
};

static de_size_table_t de_size_table[] =
{
	{ 0, 1  },      // 0 
	{ 0, 2  },      // 1 
	{ 0, 4  },      // 2 
	{ 0, 8  },      // 3 
	{ 0, 16 },      // 4 
	{ 1, 1  },      // 5 
	{ 1, 2  },      // 6 
	{ 1, 4  },      // 7 
};




//--------------------------------------
// UUIDS
//--------------------------------------

struct uuid_table_t
{
	int   uuid;
	const char* name;
} ;


static uuid_table_t uuid_table[] =
{
	{ UUID_PROTO_SDP,	        "SDP"       },
	{ UUID_PROTO_UDP,	        "UDP"       },
	{ UUID_PROTO_RFCOMM,	    "RFCOMM"    },
	{ UUID_PROTO_TCP,	        "TCP"       },
	{ UUID_PROTO_TCS_BIN,	    "TCS_BIN"   },
	{ UUID_PROTO_TCS_AT,	    "TCS_AT"    },
	{ UUID_PROTO_ATT,	        "ATT"       },
 	{ UUID_PROTO_OBEX,	        "OBEX"      },
	{ UUID_PROTO_IP,		    "IP"        },
	{ UUID_PROTO_FTP,	        "FTP"       },
	{ UUID_PROTO_HTTP,	        "HTTP"      },
	{ UUID_PROTO_WSP,	        "WSP"       },
	{ UUID_PROTO_BNEP,	        "BNEP"      },  // PAN 
	{ UUID_PROTO_UPNP,	        "UPNP"      },  
	{ UUID_PROTO_HIDP,	        "HIDP"      },  // HID 
	{ UUID_PROTO_HCRP_CTRL,     "HCRP_CTRL" },  // HCRP
	{ UUID_PROTO_HCRP_DATA,     "HCRP_DATA" },  // HCRP
	{ UUID_PROTO_HCRP_NOTE,     "HCRP_NOTE" },  // HCRP
	{ UUID_PROTO_AVCTP,	        "AVCTP"     },  // AVCTP 
	{ UUID_PROTO_AVDTP,	        "AVDTP"     },  // AVDTP 
	{ UUID_PROTO_CMTP,	        "CMTP"      },  // CIP 
	{ UUID_PROTO_UDI,	        "UDI"       },  // UDI
	{ UUID_PROTO_MCAP_CTRL,     "MCAP_CTRL" }, 
	{ UUID_PROTO_MCAP_DATA,     "MCAP_DATA" }, 
	{ UUID_PROTO_L2CAP,	        "L2CAP"     },

	{ UUID_SERVICE_SDP_SERVER,              "SDPServer"         },
	{ UUID_SERVICE_BROWSE_GROUP,	        "BrowseGroupDsc"    },
	{ UUID_SERVICE_PUBLIC_BROWSE_GROUP,	    "BrowseGroup"       },
	{ UUID_SERVICE_SERIAL_PORT,		        "SP"                },
	{ UUID_SERVICE_LAN_ACCESS,		        "LAN"               },
	{ UUID_SERVICE_DIALUP_NET,		        "DUN"               },
	{ UUID_SERVICE_IRMC_SYNC,		        "IRMCSync"          },
	{ UUID_SERVICE_OBEX_OBJ_PUSH,		    "OBEXObjPush"       },
	{ UUID_SERVICE_OBEX_FILE_TRANSFER,      "OBEXObjTrnsf"      },
	{ UUID_SERVICE_IRMC_SYNC_CMD,	        "IRMCSyncCmd"       },
	{ UUID_SERVICE_HEADSET,		            "Headset"           },
	{ UUID_SERVICE_CORDLESS_TELEPHONY,	    "CordlessTel"       },
	{ UUID_SERVICE_AUDIO_SOURCE,		    "AudioSource"       },  // A2DP 
	{ UUID_SERVICE_AUDIO_SINK,		        "AudioSink"         },  // A2DP 
	{ UUID_SERVICE_AV_REMOTE_TARGET,	    "AVRemTarget"       },  // AVRCP
	{ UUID_SERVICE_ADVANCED_AUDIO,	        "AdvAudio"          },  // A2DP 
	{ UUID_SERVICE_AV_REMOTE,		        "AVRemote"          },  // AVRCP
	{ UUID_SERVICE_VIDEO_CONFERENCING,	    "VideoConf"         },  // VCP 
	{ UUID_SERVICE_INTERCOM,		        "Intercom"          },
	{ UUID_SERVICE_FAX,			            "Fax"               },
	{ UUID_SERVICE_HEADSET_AUDIO_GATEWAY,   "HeadsetAG"         },
	{ UUID_SERVICE_WAP,			            "WAP"               },
	{ UUID_SERVICE_WAP_CLIENT,		        "WAP Client"        },
	{ UUID_SERVICE_PANU,			        "PANU"              },  // PAN 
	{ UUID_SERVICE_NAP,			            "NAP"               },  // PAN 
	{ UUID_SERVICE_GN,			            "GN"                },  // PAN 
	{ UUID_SERVICE_DIRECT_PRINTING,	        "DirectPrint"       },  // BPP 
	{ UUID_SERVICE_REFERENCE_PRINTING,	    "RefPrint"          },  // BPP 
	{ UUID_SERVICE_IMAGING,		            "Imaging"           },  // BIP 
	{ UUID_SERVICE_IMAGING_RESPONDER,	    "ImagingResponder"  },  // BIP 
    { UUID_SERVICE_IMAGING_ARCHIVE,	        "ImagingArchive"    },
    { UUID_SERVICE_IMAGING_REF_OBJS,	    "ImagingRefObjs"    },
	{ UUID_SERVICE_HANDSFREE,		        "Handsfree"         },
	{ UUID_SERVICE_HANDSFREE_AUDIO_GATEWAY, "HandsfreeAG"       },
	{ UUID_SERVICE_DIRECT_PRINTING_REF_OBJS,"RefObjsPrint"      },  // BPP 
	{ UUID_SERVICE_REFLECTED_UI,		    "ReflectedUI"       },  // BPP 
	{ UUID_SERVICE_BASIC_PRINTING,	        "BasicPrint"        },  // BPP 
	{ UUID_SERVICE_PRINTING_STATUS,	        "PrintStatus"       },  // BPP 
	{ UUID_SERVICE_HID,			            "HID"               },  // HID 
	{ UUID_SERVICE_HARDCOPY_CABLE_REPLACE,	"HCRP"              },  // HCRP
	{ UUID_SERVICE_HCR_PRINT,		        "HCRPrint"          },  // HCRP
	{ UUID_SERVICE_HCR_SCAN,		        "HCRScan"           },  // HCRP
	{ UUID_SERVICE_COMMON_ISDN_ACCESS,      "CIP"               },  // CIP 
	{ UUID_SERVICE_VIDEO_CONFERENCING_GW,   "VideoConf_GW"      },  // VCP 
	{ UUID_SERVICE_UDI_MT,		            "UDI_MT"            },  // UDI 
	{ UUID_SERVICE_UDI_TA,		            "UDI_TA"            },  // UDI 
	{ UUID_SERVICE_AUDIO_VIDEO,             "AudioVideo"        },  // VCP 
	{ UUID_SERVICE_SIM_ACCESS,		        "SAP"               },  // SAP 
	{ UUID_SERVICE_PHONEBOOK_ACCESS_PCE,	"PBAP_PCE"          },  // PBAP
	{ UUID_SERVICE_PHONEBOOK_ACCESS_PSE,	"PBAP PSE"          },  // PBAP
	{ UUID_SERVICE_PHONEBOOK_ACCESS,    	"PBAP"              },  // PBAP
    { UUID_SERVICE_MAP_MSE,		            "MAP_MSE"           },
    { UUID_SERVICE_MAP_MCE,		            "MAP_MCE"           },
    { UUID_SERVICE_MAP,			            "MAP"               },
    { UUID_SERVICE_GNSS,			        "GNSS"              },    
    { UUID_SERVICE_GNSS_SERVER,		        "GNSS_Server",      },
	{ UUID_SERVICE_PNP_INFO,		        "PNPInfo"           },
	{ UUID_SERVICE_GENERIC_NETWORKING,	    "Networking"        },
	{ UUID_SERVICE_GENERIC_FILE_TRANSGRT,   "FileTrnsf"         },
	{ UUID_SERVICE_GENERIC_AUDIO,	        "Audio"             },
	{ UUID_SERVICE_GENERIC_TELEPHONY,	    "Telephony"         },
	{ UUID_SERVICE_UPNP,			        "UPNP"              },  // ESDP
	{ UUID_SERVICE_UPNP_IP,		            "UPNP IP"           },  // ESDP
	{ UUID_SERVICE_UPNP_PAN,		        "UPNP PAN"          },  // ESDP
	{ UUID_SERVICE_UPNP_LAP,		        "UPNP LAP"          },  // ESDP
	{ UUID_SERVICE_UPNP_L2CAP,		        "UPNP L2CAP"        },  // ESDP
	{ UUID_SERVICE_VIDEO_SOURCE,		    "VideoSource"       },  // VDP 
	{ UUID_SERVICE_VIDEO_SINK,		        "VideoSink"         },  // VDP 
	{ UUID_SERVICE_VIDEO_DISTRIBUTION,	    "VideoDist"         },  // VDP 
    { UUID_SERVICE_HDP,			            "HDP"               },
    { UUID_SERVICE_HDP_SOURCE,		        "HDP_SOURCE"        },
    { UUID_SERVICE_HDP_SINK,	            "HDP_SINK"          },
	{ UUID_SERVICE_APPLE_AGENT,		        "AppleAgent"        },
};


#define UUID_TABLE_SIZE (sizeof(uuid_table)/sizeof(uuid_table_t))


//--------------------------------------
// ATTR_IDs
//--------------------------------------

struct attr_id_table_t
{
	int   attr_id;
	const char* name;
} ;


static attr_id_table_t attr_id_table[] =
{
	{ ATTR_ID_SERVICE_RECORD_HANDLE,             "SrvRecHndl"         },
	{ ATTR_ID_SERVICE_CLASS_ID_LIST,             "SrvClassIDList"     },
	{ ATTR_ID_SERVICE_RECORD_STATE,              "SrvRecState"        },
	{ ATTR_ID_SERVICE_SERVICE_ID,                "SrvID"              },
	{ ATTR_ID_PROTOCOL_DESCRIPTOR_LIST,          "ProtocolDescList"   },
	{ ATTR_ID_BROWSE_GROUP_LIST,                 "BrwGrpList"         },
	{ ATTR_ID_LANGUAGE_BASE_ATTRIBUTE_ID_LIST,   "LangBaseAttrIDList" },
	{ ATTR_ID_SERVICE_INFO_TIME_TO_LIVE,         "SrvInfoTimeToLive"  },
	{ ATTR_ID_SERVICE_AVAILABILITY,              "SrvAvail"           },
	{ ATTR_ID_BLUETOOTH_PROFILE_DESCRIPTOR_LIST, "BTProfileDescList"  },
	{ ATTR_ID_DOCUMENTATION_URL,                 "DocURL"             },
	{ ATTR_ID_CLIENT_EXECUTABLE_URL,             "ClientExeURL"       },
	{ ATTR_ID_ICON_URL,                          "IconURL"            },
	{ ATTR_ID_ADDITIONAL_PROTOCOL_DESC_LISTS,    "AdditionalProtocolDescLists" },
	{ ATTR_ID_SERVICE_NAME,                      "SrvName"            },
	{ ATTR_ID_SERVICE_DESCRIPTION,               "SrvDesc"            },
	{ ATTR_ID_PROVIDER_NAME,                     "ProviderName"       },
	{ ATTR_ID_VERSION_NUMBER_LIST,               "VersionNumList"     },
	{ ATTR_ID_GROUP_ID,                          "GrpID"              },
	{ ATTR_ID_SERVICE_DATABASE_STATE,            "SrvDBState"         },
	{ ATTR_ID_SERVICE_VERSION,                   "SrvVersion"         },
	{ ATTR_ID_SECURITY_DESCRIPTION,              "SecurityDescription"},    // PAN 
	{ ATTR_ID_SUPPORTED_DATA_STORES_LIST,        "SuppDataStoresList" },    // Synchronization 
	{ ATTR_ID_SUPPORTED_FORMATS_LIST,            "SuppFormatsList"    },    // OBEX Object Push 
	{ ATTR_ID_NET_ACCESS_TYPE,                   "NetAccessType"      },    // PAN 
	{ ATTR_ID_MAX_NET_ACCESS_RATE,               "MaxNetAccessRate"   },    // PAN 
	{ ATTR_ID_IPV4_SUBNET,                       "IPv4Subnet"         },    // PAN 
	{ ATTR_ID_IPV6_SUBNET,                       "IPv6Subnet"         },    // PAN 
	{ ATTR_ID_SUPPORTED_CAPABILITIES,            "SuppCapabilities"   },    // Imaging 
	{ ATTR_ID_SUPPORTED_FEATURES,                "SuppFeatures"       },    // Imaging and Hansfree
	{ ATTR_ID_SUPPORTED_FUNCTIONS,               "SuppFunctions"      },    // Imaging 
	{ ATTR_ID_TOTAL_IMAGING_DATA_CAPACITY,       "SuppTotalCapacity"  },    // Imaging 
	{ ATTR_ID_SUPPORTED_REPOSITORIES,            "SuppRepositories"   },    // PBAP 
};

#define ATTR_ID_TABLE_SIZE 	(sizeof(attr_id_table)/sizeof(attr_id_table_t))


const char* getUUIDName(int uuid)
{
	unsigned int i;
	for (i = 0; i < UUID_TABLE_SIZE; i++)
    {
		if (uuid_table[i].uuid == uuid)
        {
			return uuid_table[i].name;
        }
	}
	return "unknown UUID";
}


const char* getAttrIDName(int attr_id)
{
	unsigned int i;
	for (i = 0; i < ATTR_ID_TABLE_SIZE; i++)
		if (attr_id_table[i].attr_id == attr_id)
        {
            return attr_id_table[i].name;
        }
	return "unknown ATTR_ID";
}


//--------------------------------------
// Parser Atoms
//--------------------------------------

u8 parseDEHeader(sdpRequest *req, int *n)
{
	u8 de_hdr = get_u8(req);
	u8 de_type = de_hdr >> 3;
	u8 siz_idx = de_hdr & 0x07;
    
	if (de_size_table[siz_idx].addl_bits)
    {
		switch(de_size_table[siz_idx].num_bytes)
        {
            case 1: *n = get_u8(req); break;
            case 2: *n = get_u16(req); break;
            case 4: *n = get_u32(req); break;
            case 8: *n = get_u64(req); break;
		}
	}
    else
    {
		*n = de_size_table[siz_idx].num_bytes;
    }
    
    #if SHOW_DE_TYPE
        CString ts;
        ts.Format("t(0x%02x=%d,%d,%d)",de_hdr,de_type,siz_idx,*n);
        ts.Format("%-40s",(const char *)ts);
        printf((const char *)ts);
    #endif
    
	return de_type;
}


void printInt(
    u8          de_type,
    int         level,
    int         num_bytes,
    sdpRequest  *req,
    u16         *psm,
    u8          *channel)
{
	switch(de_type)
    {
        case DE_UINT:
            printf(" uint");
            break;
        case DE_INT:
            printf(" int");
            break;
        case DE_BOOL:
            printf(" bool");
            break;
	}
    
	switch(num_bytes)
    {
        case 1:         
        {
            u8 val = get_u8(req);
            printf("8  0x%02x",val);
            if (channel && de_type == DE_UINT)
                if (*channel == 0)
                    *channel = val;
            break;
        }
        case 2:          
        {
            u16 val = get_u16(req);
            printf("16 0x%04x",val);
            if (psm && de_type == DE_UINT)
                if (*psm == 0)
                    *psm = val;
            break;
        }
        case 4:         
        {
            u32 val = get_u32(req);
            printf("32 0x%08x",val);
            break;
        }
        case 8:        
        {
            u64 val = get_u64(req);
            printf("16 0x%16x",val);
            break;
        }
        case 16:
        {
            u64 val1,val2;
            get_u128(req, &val1, &val2);
            printf("128 0x%16x", val2);
            printf("%016x ", val1);
            return;
        }
        default:
        {
            printf(" err");
            inc_parse(req,num_bytes);
            return;
        }
    }
}


void printUUID(
    int         num_bytes,
    sdpRequest  *req,
    u16         *psm,
    u8          *channel)
{
	u32 uuid = 0;
	const char* s;
	switch(num_bytes)
    {
        case 2: 
            uuid = get_u16(req);
            s = "uuid-16";
            break;
        case 4: 
            uuid = get_u32(req);
            s = "uuid-32";
            break;
        case 16:
            printf(" uuid-128 ");
            for (int i = 0; i < 16; i++)
            {
                printf("%02x", ((unsigned char *) req->parse_ptr)[i]);
                if (i == 3 || i == 5 || i == 7 || i == 9)
                    printf("-");
            }
            inc_parse(req,16);
            return;
        default:
            printf(" *err*");
            inc_parse(req,num_bytes);
            return;
	}
    
	printf(" %s 0x%04x", s, uuid);
	if ((s = getUUIDName(uuid)))
		printf(" (%s)", s);
}


void printString(
    int         num_bytes,
    sdpRequest  *req,
    const char  *name)
{
	int hex = 0;
    
	for (int i=0; i< num_bytes; i++)
    {
		if (i == (num_bytes - 1) && ((char *) req->parse_ptr)[i] == '\0')
			break;
		if (!isPrint(((char *) req->parse_ptr)[i]))
        {
			hex = 1;
			break;
		}
	}
    
	printf(" %s", name);
    
	if (hex)
    {
    	for (int i=0; i< num_bytes; i++)
			printf(" %02x", ((unsigned char *) req->parse_ptr)[i]);
	}
    else
    {
		printf(" \"");
    	for (int i=0; i< num_bytes; i++)
			printf("%c", ((char *) req->parse_ptr)[i]);
		printf("\"");
	}
    
    inc_parse(req,num_bytes);
}


//---------------------------------------
// Data Elements and Lists
//---------------------------------------

void parseDE(
    int level,
    sdpRequest *req,
    int *split,
    u16 *psm,
    u8 *channel);

void parseDEs(
    bool        alt,
    u8          de_type,
    int         level,
    int         num_bytes,
    sdpRequest *req,
    int         *split,
    u16         *psm,
    u8          *channel)
{
    
    printf("%ssequence(%d)\n",
        alt ? "alt " : "",
        num_bytes);

    int start_bytes = req->bytes_parsed;        
    while (num_bytes > req->bytes_parsed - start_bytes)
    {
		parseDE(level+1, req, split, psm, channel);
    }
}


void parseDE(
    int level,
    sdpRequest *req,
    int *split,
    u16 *psm,
    u8 *channel)
{
	int num_bytes = 0;
    printIndent(level);
    
	u8 de_type = parseDEHeader(req, &num_bytes);

	switch (de_type)
    {
        case DE_NULL:
            printf(" null");
            break;
        case DE_UINT:
        case DE_INT:
        case DE_BOOL:
            printInt(de_type, level, num_bytes, req, psm, channel);
            break;
        case DE_UUID:
            if (split)
            {
                if (*split)
                    printf(" split");
                ++*split;
            }
            printUUID(num_bytes, req, psm, channel);
            break;
        case DE_URL:
        case DE_STRING:
            printString(num_bytes, req, de_type == DE_URL? "url": "str");
            break;
        case DE_SEQ:
            parseDEs(false, de_type, level, num_bytes, req, split, psm, channel);
            return;
        case DE_ALT:
            parseDEs(true,de_type, level, num_bytes, req, split, psm, channel);
            return;
	}
    printf("\n");
}



bool parseAttrList(int level, sdpRequest *req)
{
    int num_bytes = 0;
    printIndent(level);
	if (parseDEHeader(req, &num_bytes) == DE_SEQ)
    {
        printf(" attribute list(%d)\n",num_bytes);
        
        int start_bytes = req->bytes_parsed;        
        while (num_bytes > req->bytes_parsed - start_bytes)
        {
            int id_len = 0;
            printIndent(level+1);
			if (parseDEHeader(req, &id_len) == DE_UINT &&
                id_len == sizeof(u16))
            {
				u16 attr_id = get_u16(req);
                const char *name = getAttrIDName(attr_id);
				printf("attr(0x%04x,%s)\n", attr_id, name);
				int split = (attr_id != ATTR_ID_PROTOCOL_DESCRIPTOR_LIST);
                
				u16 psm = 0;
				u8  channel = 0;
				parseDE(
                    level + 2,
                    req,
                    split ? NULL : &split,
					split ? NULL : &psm,
					split ? NULL : &channel);
			}
            else
            {
                assert(0 && "ERROR: expected an Attribute ID");
				printf("\nERROR: expected an Attribute ID\n");
				printRaw(level, req);
				return false;
			}
		}
        return true;
	}

    assert(0 && "ERROR: expected a DE sequence");
    printf("\nERROR: expected a DE sequence\n");
    printRaw(level, req);
    return false;
}



//-----------------------------------------------
// high level API
//-----------------------------------------------

void parseServiceRecords(sdpRequest *req)
{
    int num_bytes = 0;
    printf("parseServiceRecords()\n");
    
    req->parse_frame_len = req->frame[0].len;
    req->parse_ptr = req->frame[0].data;
    req->parse_frame_num = 1;
    req->bytes_parsed = 0;
    
    printIndent(1);
	if (parseDEHeader(req, &num_bytes) == DE_SEQ)
    {
        printf("parsing %d bytes of service records\n",num_bytes);

        int record_num = 0;
        int start_bytes = req->bytes_parsed;        
        while (num_bytes > req->bytes_parsed - start_bytes)
        {
            printf("\n");
			printIndent(1);
            printf("Service Record(%d)\n",record_num++);
			if (!parseAttrList(2, req))
                break;
		}
	}
    else
    {
    	printIndent(1);
        printf("no service records found\n");
    }
}






#if 0
    
    static inline void print_srv_srch_pat(int level, struct frame *frm)
    {
        int len, n1 = 0, n2 = 0;
        printIndent(level);
        printf("pat");
        if (parse_de_hdr(frm, &n1) == SDP_DE_SEQ) {
            len = frm->len;
            while (len - (int) frm->len < n1 && (int) frm->len > 0) {
                if (parse_de_hdr(frm, &n2) == SDP_DE_UUID) {
                    printUUID(n2, frm, NULL, NULL);
                } else {
                    printf("\nERROR: Unexpected syntax (UUID)\n");
                    printRaw(level, frm);
                }
            }
            printf("\n");
        } else {
            printf("\nERROR: Unexpected syntax (SEQ)\n");
            printRaw(level, frm);
        }
    }
    static inline void print_attr_id_list(int level, struct frame *frm)
    {
        u16 attr_id;
        u32 attr_id_range;
        int len, n1 = 0, n2 = 0;
        printIndent(level);
        printf("aid(s)");
        if (parse_de_hdr(frm, &n1) == SDP_DE_SEQ) {
            len = frm->len;
            while (len - (int) frm->len < n1 && (int) frm->len > 0) {
                /* Print AttributeID */
                if (parse_de_hdr(frm, &n2) == SDP_DE_UINT) {
                    const char *name;
                    switch(n2) {
                    case 2:
                        attr_id = get_u16(frm);
                        name = getAttrIDName(attr_id);
                        if (!name)
                            name = "unknown";
                        printf(" 0x%04x (%s)", attr_id, name);
                        break;
                    case 4:
                        attr_id_range = get_u32(frm);
                        printf(" 0x%04x - 0x%04x",
                                (attr_id_range >> 16),
                                (attr_id_range & 0xFFFF));
                        break;
                    }
                } else {
                    printf("\nERROR: Unexpected syntax\n");
                    printRaw(level, frm);
                }
            }
            printf("\n");
        } else {
            printf("\nERROR: Unexpected syntax\n");
            printRaw(level, frm);
        }
    }


#endif


    