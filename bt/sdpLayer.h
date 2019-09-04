#ifndef _sdp_layer_h_
#define _sdp_layer_h_

#include "lcapLayer.h"

#define MAX_SDP_REQUESTS 10
#define MAX_SDP_RESULT_FRAMES  5


struct sdpResultFrame
{
    u16 txn_id;
    u8 *data;
    u16 len;
    u8 cont_len;
    u8 *cont;
};



struct sdpRequest
{
    u16 txn_id;
    u16 svc_id;
    u16 begin_attr;
    u16 end_attr;
    lcapConnection *lcn;
    u8  num_frames;
    sdpResultFrame frame[MAX_SDP_RESULT_FRAMES];

    // parser variables
    
    u16 bytes_parsed;
    u8  parse_frame_num;
    int parse_frame_len;
    u8 *parse_ptr;
};



#define MAX_SERVICES        2

struct localServiceRecord
{
    u32 service_id;
    u16 uuid_service;      // must be SDP or SP for now
    u16 uuid_protocol;      // L2CAP protocol automatically added first
    const char *service_name;
    const char *service_desc;
    u8 rfcomm_channel;      
};


class sdpLayer : public lcapClient
{
public:

    sdpLayer(lcapLayer *pLCAP);
    ~sdpLayer();
    
    sdpRequest *doSdpRequest(u8 *addr, u16 svc_id,  u16 begin_attr,  u16 end_attr );  
    
private:

    virtual u16 getPSM()  { return SDP_PSM; }
    virtual const char *serviceName() { return "SDP"; }
    virtual void lcapEvent(u16 event_type, lcapConnection *lcn,  const u8* buffer,  u16 length);
    
    sdpRequest *findRequest(lcapConnection *lcn, bool quiet);
    void deleteRequest(sdpRequest *request);
        
    u16 m_next_txn_id;
    lcapLayer *m_pLCAP;
    sdpRequest m_requests[MAX_SDP_REQUESTS];

    // rudimentary SDP server
    
    u8 m_num_services;
    u32 m_next_service_id;
    localServiceRecord m_services[MAX_SERVICES];

    u32 addService(
        u16 uuid_service,           // must be SDP or SP for now
        u16 uuid_protocol,          // L2CAP protocol automatically added first
        const char *service_name,
        const char *service_desc,
        u8 rfcomm_channel);         // set to zero if unused

    u16 sdpReply(u8 *buf, u16 cont, u16 uuid, u16 attr_low, u16 attr_high, u16 max_len);
};






#endif
