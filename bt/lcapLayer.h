
#ifndef _lcap_layer_h
#define _lcap_layer_h

#include "hciLayer.h"
#include "_lcap_defs.h"

#define TRACE_LCAP   0

#if TRACE_LCAP
    #define LOG_LCAP(f,...)     CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
    #define PRINT_LCAP(f,...)   printf(f,__VA_ARGS__)
#else
    #define LOG_LCAP(f,...)
    #define PRINT_LCAP(f,...)   
#endif



#define MAX_LCAP_CLIENTS            2
#define MAX_LCAP_CONNECTIONS        100

#define LCAP_EVENT_CONNECTING      0x0001
#define LCAP_EVENT_CONNECTED       0x0002
#define LCAP_EVENT_DISCONNECTED    0x0004
#define LCAP_EVENT_DATA            0x0010
#define LCAP_EVENT_ERROR           0x8000


extern const char *lcapEventName(u16 event);

struct lcapConnection;

class lcapClient
{
public:
    
    virtual u16 getPSM() = 0;
        // client provides us with a PSM for multiplexing packets
    virtual const char *serviceName() = 0;
        // for debugging displays
    virtual void lcapEvent(u16 event_type, lcapConnection *lcn,  const u8* buffer,  u16 length) = 0;
        // only has buffer and length for LCAP_EVENT_DATA
        // which does not include hci or lcap headers 
};




struct lcapConnection
{
    // all lcap connections are associated with a
    // particular psm (protocol multiplexer). It is
    // 0x0001 (SDP_PSM) for intial connections.
    
    u16 psm;
    
    // an lcapConnection is uniquely identified via
    // a given btAddr and local_cid, which are always
    // filled in for active connections. There is always
    // a valid pointer to an hciRemote device as weill,
    // though it may or may not have an hci_handle (yet).
    // Since all of them have local_cids, we use the absence
    // of a cid to indicate an empty slot.

    u16 local_cid;
    hciRemoteDevice *device;

    // the state is set during the connection process
    // connections get remote_cids during the connection
    // process and are guaranteed to have one in STATE_CONNECTED

    u8  lcap_state;
    u16 remote_cid;
    
    // for connections succesfully opened to clients
    // we cache a pointer to the client for speed
    
    lcapClient *pClient;
    
};





class lcapLayer : public hciClient
{
public:
    
	lcapLayer(hciLayer *pHCILayer);
	~lcapLayer (void);

    // client API
    
    hciLayer *getHCILayer()  { return m_pHCI; }
    void sendData(const void *buffer, unsigned length);
        // sets the length word
        // prh - this should be private as it assumes
        // client knows hci_handle and lcap_cids ...
    
    void registerClient(lcapClient *pClient);
        // SDP and RFCOMM at this time
        
    lcapConnection *startConnection(u8 *addr, u16 psm);
        // Call this method to start an lcap connection
        // to the given machine on the provided PSM.
        // The pointer is valid until a DISCONNECTED
        // or ERROR event.  Note that the psm must
        // map to the psm of a registered service
        // in order for any data to be received.
    void closeConnection(lcapConnection *lcn);
        // you should close the connection and
        // invalidate your pointer when you are
        // done with the connection.
        
    // void sendLcapData(lcapConnection *lcn, const u8 *buffer, const u16 length);
        // wrap and send the given data packet.
        
        
private:

	hciLayer *m_pHCI;

    u16 m_num_clients;
    lcapClient *m_pClients[MAX_LCAP_CLIENTS];
    lcapConnection m_connections[MAX_LCAP_CONNECTIONS];

    void closeHCIConnection(hciRemoteDevice *device);
    lcapConnection *findRemoteCid(hciRemoteDevice *device, u16 cid);
    lcapConnection *findLocalCid(u16 hci_handle, u16 cid);  
    
    lcapConnection *addConnection(hciRemoteDevice *device, u16 psm, u16 rcid);
    void deleteConnection(lcapConnection *lcn,  bool ifLastCloseHCI);

    // client supprt methods
    
	lcapClient *findClientByPSM(u16 psm);
    void dispatchEvents(hciRemoteDevice *device, u16 event_type);

    // virtual public calls UP from hciLayer
    
    virtual void receiveData(const void *buffer, unsigned length);
    virtual void receiveEvent(u16 hci_client_event, hciRemoteDevice *device);

    // connection process support
    
    void checkLcapConnectionState(lcap_command_packet_header *req, lcapConnection *lcn);
    
};


#endif
