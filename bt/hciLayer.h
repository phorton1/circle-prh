#ifndef _hcilayer_h
#define _hcilayer_h

#define HCI_USE_FAT_DATA_FILE  1

#include "_hci_defs.h"
#include "btQueue.h"
#include "transportUart.h"
#include "hciBase.h"
#include "hciVendor.h"
#include <circle/types.h>
#include <circle/interrupt.h>
#include <circle/usb/usbbluetooth.h>
#include <circle/string.h>

#if HCI_USE_FAT_DATA_FILE
	#include <fatfs/ff.h>	// use the addon library
#endif

#define TRACE_HCI  0

#if TRACE_HCI
    #define LOG_HCI(f,...)   CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
#else
    #define LOG_HCI(f,...)
#endif


// these bits are kept in the high byte of the nci_handle
// a non-zero handle with neither set is valid

#define HCI_HANDLE_CONNECTING        0x1000
#define HCI_HANDLE_ERROR             0x8000

// hci client events

#define HCC_EVENT_CONNECTED          0x0001
#define HCC_EVENT_DISCONNECTED       0x0002
#define HCC_EVENT_CONNECTION_ERROR   0x8000
#define HCC_INQUIRY_COMPLETE         0x0100
#define HCC_INQUIRY_DEVICE_FOUND     0x0200
#define HCC_INQUIRY_NAME_FOUND       0x0400


#define HCI_DEVICE_INCLUDE_UNUSED_FIELDS   0


struct hciRemoteDevice
{
    hciRemoteDevice *prev;
    hciRemoteDevice *next;
    
    u8      addr[BT_ADDR_SIZE];
    u8      device_class[BT_CLASS_SIZE];
    char    name[BT_NAME_SIZE];
    u16     handle;
    u8      link_key_type;
    u8      link_key[BT_LINK_KEY_SIZE];
    
    // support for lcap layer
    
    u16     next_lcap_id;
    u16     next_lcap_cid;
    
    #if HCI_DEVICE_INCLUDE_UNUSED_FIELDS
        u16     packet_type;
        u8      page_rep_mode;
        u8      page_mode;
        u16     clock_offset;
        u8      max_slots;
        u16     timeout;
        u8      link_type;
        u8      encrypt;
        u8      bonding_state;
    #endif
   
};


class hciClient
{
    public:
    virtual void receiveData(const void *buffer, unsigned length) = 0;
    virtual void receiveEvent(u16 hci_client_event, hciRemoteDevice *device) = 0;
};


class hciLayer
{
public:
    
	hciLayer();
	~hciLayer(void);

    void registerAsClient(hciClient *pClient)
        { m_pClient = pClient; }
    
    btTransportBase *getTransport()
        { return m_pTransport; }
        
	bool initialize(btTransportBase *pTransport
        #if HCI_USE_FAT_DATA_FILE
            ,FATFS *pFileSystem
        #endif
        );
    bool isSetup() { return m_HCIBase.isSetup(); }

	void sendData(const void *buffer, unsigned length);
	void sendCommand(const void *buffer, unsigned length);
        // sets the hci length byte/word
    
	hciRemoteDevice *startConnection(u8 *addr);
    void closeConnection(hciRemoteDevice *device);
    
    void startInquiry(unsigned nSeconds);
    void cancelInquiry();

    u16 getNumDevices() { return m_num_devices; }
    hciRemoteDevice *getDeviceList() { return m_first_device; }
    
    hciRemoteDevice *addDevice(const u8 *bdAddr, const char *name);
    hciRemoteDevice *findDeviceByAddr(const u8 *addr);
    hciRemoteDevice *findDeviceByName(const char *name);
    hciRemoteDevice *findDeviceByHandle(u16 handle);
    
    void unpair(hciRemoteDevice *device); 
    
    #if HCI_USE_FAT_DATA_FILE
        void saveDevices();
    #endif
    
    
private:
friend class btTask;
    
    u16 m_num_devices;  
    hciRemoteDevice *m_first_device;
    hciRemoteDevice *m_last_device;

    void init();
	void process(void);
    void processEvent(const void *buffer, unsigned length);

    void receiveTransport(u8 hci_prefix, const void *buffer, unsigned length);
	static void receiveTransportStub(void * pHCILayer, u8 hci_prefix, const void *buffer, unsigned length);

	hciBase             m_HCIBase;
    hciClient           *m_pClient;
    CInterruptSystem    *m_pInterrupt;
    btTransportBase     *m_pTransport;
	hciVendor           *m_pVendor;

	btQueue m_command_queue;
	btQueue m_event_queue;
	btQueue m_send_data_queue;
	btQueue m_recv_data_queue;

    u16 m_length;
    u16 m_offset;
    u8  m_packet_prefix;
    u16 m_can_send_data;
    u16 m_can_send_command;
    
    // must be aligned to work with USB dwhcixferstagedata.cpp
    
    btBuffer *m_pBuffer;
    // u8  m_buffer[HCI_MAX_PACKET_SIZE] __attribute__ ((aligned (4)));
        // for the time being HCI is handling 3 kinds of packets,
        // two of which are limited by the protocol in size.
        // We are currently arbitrariliy limiting data packets
        // when their maximum size *should* be determined after
        // using a HCI command HCI_OP_INFORM_READ_BUFFER_SIZE
        // to get the value from the hardware.
        
    void removeDevice(hciRemoteDevice *device);
    bool checkDeviceName(hciRemoteDevice *device);
    
    u16 m_next_hci_handle;
    u16 m_num_name_requests;
    
    
    #if HCI_USE_FAT_DATA_FILE
        FATFS *m_pFileSystem;
        void loadDevices();
    #endif
        
};


#endif
