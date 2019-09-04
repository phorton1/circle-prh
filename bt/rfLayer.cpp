
#include "rfLayer.h"
#include <circle/util.h>
#include <circle/timer.h>


#define log_name   "rfcomm"

#define INCOMING_CONNECTIONS       1


// PROTOCOL STEPS
//
//   Outgoing connections are largely driven by the local machine.
//   Incoming connections are largely driven by the remote machine.
//
//   Apart from the SABMs for DLC0, the rest of the steps happen to
//   the destination channel (P10=0x2, lenovo=0x3, mySPP=0x11).
//
// 	   OUTGOING
//
//         send SABM 0
//         recv UA 0  
//         send PN command
//         recv PN response
//         send SABM n
//         recv UA n
//         send MSC command
//         recv MSC response
//         recv MSC command
//         send MSC response
//         send RPN command
//         recv RPN response
//         OPEN
//
//    INCOMING
//
//         recv SABM 0
//         send UA 0
//         recv PN command (optional)
//         send PN response (optional)
//         recv MSC command
//         send MSC response
//         send MSC command
//         recv MSC response
//         recv RPN command (optional)
//         send RPN response (optional)
//         OPEN
//
//   For outgoing connections, the PN and RPN cycles are built in and required.
//   For incoming connections, the PN and RPN cycles are optional ...
//       we respond to the commands if we receive them,
//       and they can be received at any time
//       but we consider the port open once we have done the MSC's

	
#define DO_DOUBLE_THE_MSC  0
	// at one point I added a second pair of MSC
	// negotiations cuz that's what the lenovo does

struct rf_frame
{
    u8 address; 
    u8 control;
    u8 len;
}   PACKED;

struct rf_command
{
    lcap_data_packet_header hdr;
    rf_frame frame;
}   PACKED;

struct dpnParameters
{
    u8  DLCI;				// shifted by 1 (direction bit)
    u8  FrameType:4;		// as the code example sets it zero, but I want 0xf0 for the lenovo
    u8  ConvergenceLayer:4;	// the only thing that made sense was to switch these ... 
    u8  Priority;
    u8  ACKTimerTicks;
    u16 MaximumFrameSize;
    u8  MaxRetransmissions;
    u8  RecoveryWindowSize;
}   PACKED;

struct dpnCommand_t
{
    u8  cmd_byte;
    u8  len;
    dpnParameters params;
}   PACKED;



struct rpnParameters
{
    u8  DLCI;				// shifted by 3 (EA,CR, & direction bit)
    u8  baud_rate;
	u8  data_stop_parity;
	u8  flow_control;
	u8  xon;
	u8  xoff;
	u16 parameter_mask;
}   PACKED;

struct rpnCommand_t
{
    u8  cmd_byte;
    u8  len;
    rpnParameters params;
}   PACKED;



//------------------------------------------------------
// defines and constants
//------------------------------------------------------

#define RF_POLL_FINAL  (1 << 4)

#define	RF_FRAME_DM    0x0F    // Disconnected Mode
#define	RF_FRAME_DISC  0x43    // Disconnect 
#define	RF_FRAME_SABM  0x2F    // Set Asynchronous Balance Mode 
#define	RF_FRAME_UA    0x63    // Unnumbered Acknowledgement 
#define	RF_FRAME_UIH   0xEF    // Unnumbered Information with Header check 

#define RF_SIGNAL_FC                        (1 << 1)
#define RF_SIGNAL_RTC                       (1 << 2)
#define RF_SIGNAL_RTR                       (1 << 3)
#define RF_SIGNAL_IC                        (1 << 6)
#define RF_SIGNAL_DV                        (1 << 7)
            
#define RF_CONFIG_REMOTESIGNALS             (1 << 0)
#define RF_CONFIG_LOCALSIGNALS              (1 << 1)
#define RF_CONFIG_LOCALSIGNALSSENT          (1 << 2)
#define RF_CONFIG_ABMMODESET                (1 << 3)

#define RF_CONTROL_TEST                         (0x20 >> 2)
#define RF_CONTROL_FLOW_CONTROL_ENABLE          (0xA0 >> 2)
#define RF_CONTROL_FLOW_CONTROL_DISABLE         (0x60 >> 2)
#define RF_CONTROL_MODEM_STATUS                 (0xE0 >> 2)
#define RF_CONTROL_REMOTE_PORT_NEGOTIATION      (0x90 >> 2)
#define RF_CONTROL_REMOTE_LINE_STATUS           (0x50 >> 2)
#define RF_CONTROL_DLC_PARAMETER_NEGOTIATION    (0x80 >> 2)
#define RF_CONTROL_NON_SUPPORTED_COMMAND        (0x10 >> 2)


// 0th byte of RPN

#define RF_RPN_BAUD_2400 		0x00	// bit/s 0 0 0 0 0 0 0 0
#define RF_RPN_BAUD_4800 		0x01	// bit/s 1 0 0 0 0 0 0 0
#define RF_RPN_BAUD_7200 		0x02	// bit/s 0 1 0 0 0 0 0 0
#define RF_RPN_BAUD_9600 		0x03	// bit/s 1 1 0 0 0 0 0 0
#define RF_RPN_BAUD_19200 		0x04	// bit/s 0 0 1 0 0 0 0 0
#define RF_RPN_BAUD_38400 		0x05	// bit/s 1 0 1 0 0 0 0 0
#define RF_RPN_BAUD_57600 		0x06	// bit/s 0 1 1 0 0 0 0 0
#define RF_RPN_BAUD_115200 		0x07	// bit/s 1 1 1 0 0 0 0 0
#define RF_RPN_BAUD_230400 		0x08	// bit/s 0 0 0 1 0 0 0 0

// 1st byte of RPN

#define RF_RPN_DATA_BITS_MASK	0x03		// 11 8 bits - default
#define RF_RPN_DATA_BITS_5		0x00		// 00 5 bits
#define RF_RPN_DATA_BITS_6		0x01		// 01 6 bits
#define RF_RPN_DATA_BITS_7		0x02		// 10 7 bits
#define RF_RPN_DATA_BITS_8		0x03		// 11 8 bits - default

#define RF_RPN_STOP_BIT         0x04		// S=0: 1 stop bit, S=1: 1,5 stop bits
#define RF_RPN_PARITY           0x08

#define RF_RPN_PARITY_MASK      0x30
#define RF_RPN_PARITY_ODD       0x00
#define RF_RPN_PARITY_EVEN      0x10
#define RF_RPN_PARITY_BREAK     0x20
#define RF_RPN_PARITY_SPACE     0x30

// 2nd byte of RPN

#define RF_RPN_FLOW_X_INPUT     0x01	// Bit1 XON/XOFF on input
#define RF_RPN_FLOW_X_OUTPUT    0x02	// Bit2 XON/XOFF on output
#define RF_RPN_FLOW_RTR_INPUT   0x04	// Bit3 RTR on input
#define RF_RPN_FLOW_RTR_OUTPUT  0x08	// Bit4 RTR on output
#define RF_RPN_FLOW_RTC_INPUT   0x10	// Bit5 RTC on input
#define RF_RPN_FLOW_RTC_OUTPUT  0x20	// Bit6 RTC on output


// parameter mask

#define RF_RPN_MASK_BIT_RATE		0x0001	// Bit1 bit rate
#define RF_RPN_MASK_DATA_BITS		0x0002	// Bit2 data bits
#define RF_RPN_MASK_STOP_BITS		0x0004	// Bit3 stop bits
#define RF_RPN_MASK_PARITY			0x0008	// Bit4 Parity
#define RF_RPN_MASK_PARITY_TYPE		0x0010	// Bit5 parity type
#define RF_RPN_MASK_XON_CHAR		0x0020	// Bit6 XON character
#define RF_RPN_MASK_XOF_CHAR		0x0040	// Bit7 XOF character
#define RF_RPN_MASK_X_INPUT			0x0100	// Bit1 XON/XOFF on input
#define RF_RPN_MASK_X_OUTPUT		0x0200	// Bit2 XON/XOFF on output
#define RF_RPN_MASK_RTR_INPUT		0x0400	// Bit3 RTR on input
#define RF_RPN_MASK_RTR_OUTPUT		0x0800	// Bit4 RTR on output
#define RF_RPN_MASK_RTC_INPUT		0x1000	// Bit5 RTC on input
#define RF_RPN_MASK_RTC_OUTPUT		0x2000	// Bit6 RTC on output


const u8 crc_table[256] =
{
    0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75, 0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
    0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69, 0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
    0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D, 0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
    0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51, 0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
    0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05, 0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
    0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19, 0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
    0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D, 0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
    0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21, 0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
    0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95, 0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
    0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89, 0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
    0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD, 0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
    0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1, 0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
    0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5, 0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
    0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9, 0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
    0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD, 0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
    0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1, 0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF
};


u8 FCS(u8 *p, u8 len)
{
	u8 fcs = 0xFF;
    while (len--)
        fcs = crc_table[fcs ^ *p++];
    return 0xff - fcs;
}

const char *frameTypeName(u8 frame_type)
{
    if (frame_type == RF_FRAME_DM  ) return "DM";
    if (frame_type == RF_FRAME_DISC) return "DISC";
    if (frame_type == RF_FRAME_SABM) return "SABM";
    if (frame_type == RF_FRAME_UA  ) return "UA";
    if (frame_type == RF_FRAME_UIH ) return "UIH";
    return "UNKNOWN RF FRAME TYPE!!";
}

const char *controlTypeName(u8 control_type)
{
    if (control_type == RF_CONTROL_TEST                     ) return "TEST";                     
    if (control_type == RF_CONTROL_FLOW_CONTROL_ENABLE      ) return "FLOW_CONTROL_ENABLE";
    if (control_type == RF_CONTROL_FLOW_CONTROL_DISABLE     ) return "FLOW_CONTROL_DISABLE";     
    if (control_type == RF_CONTROL_MODEM_STATUS             ) return "MODEM_STATUS";    
    if (control_type == RF_CONTROL_REMOTE_PORT_NEGOTIATION  ) return "REMOTE_PORT_NEGOTIATION";
    if (control_type == RF_CONTROL_REMOTE_LINE_STATUS       ) return "REMOTE_LINE_STATUS";
    if (control_type == RF_CONTROL_DLC_PARAMETER_NEGOTIATION) return "DLC_PARAMETER_NEGOTIATION";
    if (control_type == RF_CONTROL_NON_SUPPORTED_COMMAND    ) return "NON_SUPPORTED_COMMAND";
    return "UNKNOWN CONTROL TYPE";
}


//-------------------------------------------
// code
//-------------------------------------------


rfcommLayer::rfcommLayer(lcapLayer *pLCAP, sdpLayer *pSDP) :
    m_pLCAP(pLCAP),
    m_pSDP(pSDP)
{
    memset(m_sessions,0,RF_MAX_SESSIONS * sizeof(rfSession));
    m_pClient = 0;
    m_pClientThis = 0;    
}


rfcommLayer::~rfcommLayer()
{
    m_pClient = 0;
    m_pClientThis = 0;    
    m_pSDP = 0;
    m_pLCAP = 0;
}


//--------------------------------------
// utils
//--------------------------------------


rfSession *rfcommLayer::findSession(lcapConnection *lcn)
{
    for (u16 i=0; i<RF_MAX_SESSIONS; i++)
    {
        if (lcn == m_sessions[i].lcn)
        {
            return &m_sessions[i];
        }
    }
    LOG_ERROR("Could not find rfSession(0x%04x)",lcn->local_cid);
    return 0;
}


rfSession *rfcommLayer::addSession(lcapConnection *lcn)
{
    assert(lcn);
    LOG("addSession(0x%04x)",lcn->local_cid);
    for (u16 i=0; i<RF_MAX_SESSIONS; i++)
    {
        if (!m_sessions[i].lcn)
        {
            rfSession *session = &m_sessions[i];
            memset(session,0,sizeof(rfSession));
            session->lcn = lcn;
            return session;
        }
    }
    LOG_ERROR("no room for more sessions on lcn lcid(0x%04x)",lcn->local_cid);
    return 0;
}


void rfcommLayer::deleteSession(rfSession *session)
{
    assert(session && session->lcn);
    LOG("deleteSession(0x%04x)",session->lcn->local_cid);
    memset(session,0,sizeof(rfSession));
}



rfChannel *rfcommLayer::findChannel(rfSession *session, u8 channel_num)
{
    for (u16 i=0; i<RF_MAX_CHANNELS; i++)
    {
        if (session->channel[i].session &&
            session->channel[i].channel_num == channel_num)
        {
            return &session->channel[i];
        }
    }
    LOG_ERROR("Could not find channel(%d) for session(0x%04x)",
        channel_num,session->lcn->local_cid);
    return 0;
}



rfChannel *rfcommLayer::addChannel(rfSession *session, u8 channel_num)
{
    assert(session && session->lcn);
    LOG("addChannel(0x%04x,%d)",session->lcn->local_cid,channel_num);
    for (u16 i=0; i<RF_MAX_CHANNELS; i++)
    {
        if (!session->channel[i].session)
        {
            rfChannel *channel = &session->channel[i];
            memset(channel,0,sizeof(rfChannel));
            channel->session = session;
            channel->channel_num = channel_num;
            channel->local.signals  = RF_SIGNAL_RTC | RF_SIGNAL_RTR | RF_SIGNAL_DV;
			
			// inherit the parent sessions RF_CHANNEL_STATE_INCOMING bit
			// even though that's not right .. it's incoming any time
			// except when it's outgoing (which could mark instead)
			
			if (channel_num)
			{
				rfChannel *dlc0 = &session->channel[0];
				if (dlc0->channel_state & RF_CHANNEL_STATE_INCOMING)
					channel->channel_state |= RF_CHANNEL_STATE_INCOMING;
			}
			
			channel->priority = 7;		// 7 is an arbitrary priority
			channel->mtu      = 0x200;  // 512 byte MTU
			
            return channel;
        }
    }
    LOG_ERROR("no room for more channels in session(0x%04x)",session->lcn->local_cid);
    return 0;
}


void rfcommLayer::deleteChannel(rfChannel *channel)
{
    assert(channel && channel->session && channel->session->lcn);
    LOG("deleteChannel(0x%04x,%d)",channel->session->lcn->local_cid,channel->channel_num);
    memset(channel,0,sizeof(rfChannel));
}


void rfcommLayer::sendPacket(rfChannel *channel, bool command, u8 frame_type, u8 *data, u16 data_len)
{
    assert(data_len < 128);
        // otherwise we'd need to use the len EA bit
    assert(channel->session->lcn);
    lcapConnection *lcn = channel->session->lcn;

    #define MAX_RFCOMM_FRAME 256
    u8 buf[MAX_RFCOMM_FRAME];
    memset(buf,0,MAX_RFCOMM_FRAME);

    u8 addr_bits = command ? 0x03 : 0x01;
    u8 poll_bit = command && frame_type != RF_FRAME_UIH ? RF_POLL_FINAL : 0;
    
    rf_command *cmd = (rf_command *) buf;
    cmd->hdr.hci.handle = lcn->device->handle | 0x2000;
    cmd->hdr.lcap_d.cid = lcn->remote_cid;
    cmd->frame.address = (channel->channel_num << 3) | addr_bits;
        // bits 0   : EA=1 (the low order bit is always set int headdress)
        //      1   : Command(1) or Response(0)
        //      2   : Direction initiator(1) or responder (0)
        //      3-7 : channel_num
    cmd->frame.control = frame_type | poll_bit;
        // the poll bit (0x10) is what changes the constant
        // RF_FRAME_SABM = 0x2f into what we send 0x3f
    cmd->frame.len = 0x01 | (data_len << 1);
        // also has an EA bit which could be used
        // otherwise there would be data here
        // data - 0 bytes
    
    // copy the data
    
    u8 *data_ptr = &buf[sizeof(rf_command)];
    if (data_len)
    {
        memcpy(data_ptr,data,data_len);
        data_ptr += data_len;
    }
    
    // add the FCS byte and calculate packet_len
    
    u16 crc_len = frame_type == RF_FRAME_UIH ? 2 : 3;   // len + 3;
    *data_ptr++ = FCS((u8*)&cmd->frame, crc_len);
    u16 packet_len = ((u32)data_ptr) - ((u32)buf);

    LOG("sendPacket channel(%d) frame_type(0x%02x:%s) data_len(%d) packet_len(%d)",
        channel->channel_num,
        frame_type,
        frameTypeName(frame_type),
        data_len,
        packet_len);
    display_bytes("<rfcomm",(const u8*)&cmd->frame, data_len+4); // (const u8 *) buf,packet_len);

    // send it
    
    m_pLCAP->sendData(buf,packet_len);
    
}   // rfCommLayer::sendPacket();


//--------------------------------------------------
// LCAP EVENT HANDLER
//--------------------------------------------------

void rfcommLayer::lcapEvent(u16 event_type, lcapConnection *lcn,  const u8* buffer,  u16 length)
{
	assert(lcn->device);
	
    LOG("rfcommLayer::event(%s) hci(0x%04x) cid(0x%04x) len=%d",
        lcapEventName(event_type),
        lcn->device ? lcn->device->handle : 0xffff,
        lcn->local_cid, length);
    
    switch (event_type)
    {
        case LCAP_EVENT_DISCONNECTED :
        {
            rfSession *session = findSession(lcn);
            if (!session) return;
            deleteSession(session);
			break;
		}
        
        case LCAP_EVENT_CONNECTED :
        {
            rfSession *session = findSession(lcn);
            if (!session)
            #if INCOMING_CONNECTIONS
                session = addSession(lcn);               
            #else
                return;
            #endif
            
            rfChannel *channel = findChannel(session,0);
            if (!channel)
	    
            #if INCOMING_CONNECTIONS
            {
                channel = addChannel(session,0);
                channel->channel_state |= RF_CHANNEL_STATE_INCOMING;
            }
            #else
                return;
            #endif
            
            processChannel(channel);
            break;
        }
        
        case LCAP_EVENT_DATA :
        {
            display_bytes(">rfcomm",buffer,length);
            rfEventHandler(lcn,buffer,length);
            break;
        }
           
        default:
            LOG_ERROR("unhandled RFCOMM LCAP EVENT(%s) hci(0x%04x) cid(0x%04x) len=%d",
                lcapEventName(event_type),lcn->device->handle, lcn->local_cid, length);
            break;
            
    }   // switch(LCAP_EVENT_TYPE)
}   // rfcommLayer::lcapEvent()



//-------------------------------------------------------
// RFCOMM Event Handler
//-------------------------------------------------------

void rfcommLayer::rfEventHandler(lcapConnection *lcn, const u8 *buffer, u16 length)
{
    rf_frame *p_frame = (rf_frame *) buffer;
    u8 frame_type = p_frame->control & ~RF_POLL_FINAL;
    u8 channel_num = p_frame->address >> 3;
    
    LOG("rfEventHandler(0x%02x:%s,%d) length=%d",
        frame_type,
        frameTypeName(frame_type),
        channel_num,
        length);
    
    rfSession *session = findSession(lcn);
    if (!session) return;
    rfChannel *channel = findChannel(session,channel_num);
    if (!channel)
    #if INCOMING_CONNECTIONS
        channel = addChannel(session,channel_num);
    #else
        return;
    #endif
        
    // verify (that I understand) the FCS
    
    #if 0
        u8 fcs = buffer[length-1];
        u8 calculated = FCS((u8*)buffer,frame_type==RF_FRAME_UIH?2:3);
        if (fcs != calculated)
        {
            LOG_ERROR("unexpected fcs(%02x) calculated(%02x)",fcs,calculated);
        }
        else
        {
            LOG("FCS matches fcs(%02x) calculated(%02x)",fcs,calculated);
        }
    #endif

    switch (frame_type)
    {
	case RF_FRAME_SABM :
	{
	    LOG("received SABM for channel(%d)",channel_num);
	    // connection attempt from remote device
	    // send the UA, and mark SABM and UA done
	    LOG("sending UA for channel(%d)",channel_num);
	    channel->channel_state |= RF_CHANNEL_STATE_SABM;
	    channel->channel_state |= RF_CHANNEL_STATE_UA;
	    sendPacket(channel,true,RF_FRAME_UA,0,0);
	    return;
	}
        case RF_FRAME_UA  :
        {
            LOG("received UA for channel(%d)",channel_num);
            channel->channel_state |= RF_CHANNEL_STATE_UA;
            break;
        }
        case RF_FRAME_UIH :
        {
            u8 len = p_frame->len >> 1;
            u8 *data = (u8*)&buffer[sizeof(rf_frame)];
            if (channel->channel_num)
            {
                display_bytes(">UIH SPP",data,len);
                if (m_pClient)
                {
                    (*m_pClient)(m_pClientThis, channel, data, len);

					// we don't need to process channels in the case of
					// UIH data packets to clients (at this point)
					// so we return here.
		
					return;
                }
            }
            else    //if to channel 0, it's a control frame ...
            {
                u8 c_command = data[0] & 0x02;
                u8 c_type = data[0] >> 2;       // type = 0xe3 >> 2 == 0x38
                u8 c_len = data[1] >> 1;        // len = 2
                    
                LOG("DLC0 control type(0x%02x==>0x%02x:%s) %s len 0x%02x=>%d",
                    data[0],
                    c_type,
                    controlTypeName(c_type),
                    c_command ? "COMMAND" : "RESPONSE",
                    data[1],
                    c_len);
                display_bytes(">>rfctrl",&data[2],c_len);
                
                switch (c_type)
                {
                    case RF_CONTROL_MODEM_STATUS :
                        controlModemStatus(channel,c_command,&data[2],c_len);
                        break;
            
					case RF_CONTROL_DLC_PARAMETER_NEGOTIATION :
						controlParamNegotiation(channel,c_command,&data[2],c_len);
						break;

                    case RF_CONTROL_REMOTE_PORT_NEGOTIATION :
						controlRemotePortNegotiation(channel,c_command,&data[2],c_len);
						break;
					
                    case RF_CONTROL_TEST :                          
                    case RF_CONTROL_FLOW_CONTROL_ENABLE :
                    case RF_CONTROL_FLOW_CONTROL_DISABLE :      
                    case RF_CONTROL_REMOTE_LINE_STATUS :  
                    case RF_CONTROL_NON_SUPPORTED_COMMAND :
                    default: 
                        LOG_ERROR("unsupported control type(0x%02x:%s) %s len=%d\n",
                            c_type,
                            controlTypeName(c_type),
                            c_command ? "command" : "response",
                            c_len);
                        break;
                }
            }
            break;
        }
        
        default:
        case RF_FRAME_DM :
        case RF_FRAME_DISC :
        {
            LOG("unhandled frame type (0x%02x:%s) for channel(%d)",
                frame_type,
                frameTypeName(frame_type),
                channel_num);
            break;
        }
    }

    // process the channels after any events
    
    for (u16 i=0; i<RF_MAX_CHANNELS; i++)
    {
	if (session->channel[i].session)
	    processChannel(&session->channel[i]);
    }

}   // rfEventHandler()






//===================================
// processChannel
//===================================

void rfcommLayer::processChannel(rfChannel *channel)
{
    assert(channel);
    
    // DLC0 receives the minimum of processing ..
    // if it's not incoming and we havn't done so
    // already, all we do is send the SABM packet,
    // that's it, replete with short ending ..
    
    if (!channel->channel_num)
    {
		if (!(channel->channel_state & RF_CHANNEL_STATE_INCOMING) &&
			!(channel->channel_state & RF_CHANNEL_STATE_SABM))
		{
			LOG("sending SABM for DLC0",0);
			sendPacket(channel,true,RF_FRAME_SABM,0,0);
			channel->channel_state |= RF_CHANNEL_STATE_SABM;
		}
		return;
    }
    
    // otherwise, process the state machine for channel 'n'
    // using channel 0, but only if it dlc0 is SABM'd and UA'd
    // otherwise, another short ending
    
    rfChannel *dlc0 = findChannel(channel->session,0);
    if (!dlc0) return;	// this is an error return
    
    if (!(dlc0->channel_state & RF_CHANNEL_STATE_SABM) ||
		!(dlc0->channel_state & RF_CHANNEL_STATE_UA))
		return;	// this is a normal 'waiting for ...' return
    
    // one more short ending (for now) ... don't go through the
	// state machine anymore once a channel is OPEN.  We WILL
	// continue respond to incoming commands, but we no longer
	// pro-actively send any for this channel ...
    
    if (channel->channel_state & RF_CHANNEL_STATE_OPEN)
    {
		LOG_ERROR("processChannel(%d) state machine stopped for OPEN channel!",
			channel->channel_num);
		return;
    }


    //---------------------------------------------------------------
    // we are now processing the channel->channel_num state machine
    //---------------------------------------------------------------
    // we set and clear bits and use dlc0 to send control commands.
	// starting with an outgoing PN parameter negotiation command if needed
	
	if (!(channel->channel_state & RF_CHANNEL_STATE_PN_COMMAND_SENT))
	{
		LOG("sending PN param negotiation for channel(%d)",
			channel->channel_num);
		
		// we only maintain, and actively set, two of these parameters,
		// the mtu to 0x200 and priority to 7, during addChannel()
		// otherwise, everything else is a constant ...
		
		dpnCommand_t dpnCommand;
		dpnCommand.cmd_byte = (RF_CONTROL_DLC_PARAMETER_NEGOTIATION << 2) | 3;
		dpnCommand.len = (sizeof(dpnParameters) << 1) | 0x01;
		dpnCommand.params.DLCI 					= (channel->channel_num << 1);
		dpnCommand.params.FrameType 			= 0;	
		dpnCommand.params.ConvergenceLayer 		= 0;
		dpnCommand.params.Priority 				= channel->priority;			
		dpnCommand.params.ACKTimerTicks 		= 0;
		dpnCommand.params.MaximumFrameSize 		= channel->mtu;	
		dpnCommand.params.MaxRetransmissions 	= 0;
		dpnCommand.params.RecoveryWindowSize 	= 7;			
		
		bool is_response = channel->channel_state & RF_CHANNEL_STATE_INCOMING;
		sendPacket(dlc0,!is_response,RF_FRAME_UIH,(u8 *) &dpnCommand,sizeof(dpnCommand));
		channel->channel_state |= RF_CHANNEL_STATE_PN_COMMAND_SENT;
	}
	else if (!(channel->channel_state & RF_CHANNEL_STATE_PN_RESPONSE_RECEIVED))
	{
		// do nothing, we are waiting for a PN response
		// to our outgoing PN command
	}
    
	else if (!(channel->channel_state & RF_CHANNEL_STATE_INCOMING) &&
			 !(channel->channel_state & RF_CHANNEL_STATE_SABM))
    {
		// send the SABM packet for channel 'n'
		LOG("sending SABM packet for channel(%d)",
			channel->channel_num);
		sendPacket(channel,true,RF_FRAME_SABM,0,0);
		channel->channel_state |= RF_CHANNEL_STATE_SABM;
    }
    
    else if (!(channel->channel_state & RF_CHANNEL_STATE_UA))
    {
		// do nothing, we are waiting for a UA response
		// to our outgoing SABM or the incoming SABM
		// and the UA we send out ...
    }
    
	
	// MSC negotiation
	// for these, we drive the incoming connection as well
	
    else if (!(channel->channel_state & RF_CHANNEL_STATE_SIGNALS_SENT))
    {
		// send the signals packet for channel 'n'
		LOG("sending MSC signals for channel(%d)",
			channel->channel_num);
			sendSignals(channel);
	}
    else if (!(channel->channel_state & RF_CHANNEL_STATE_SIGNAL_RESPONSE_RECEIVED))
    {
		// do nothing, we are waiting for a signals response
    }
    else if (!(channel->channel_state & RF_CHANNEL_STATE_SIGNALS_RECEIVED))
    {
		// do nothing, we are waiting for a signals command from the other guy
    }
    else if (!(channel->channel_state & RF_CHANNEL_STATE_SIGNAL_RESPONSE_SENT))
    {
		// never gets here since we send the signal response as soon
		// as we receive the signals from the other guy
    }
	
    
	// OUTGOING RPN send our port setting
	// this step is not optional for either incoming or outgoing connections

	else if (!(channel->channel_state & RF_CHANNEL_STATE_RPN_COMMAND_SENT))
	{
		LOG("sending RPN param negotiation for channel(%d)",
			channel->channel_num);
		
		rpnCommand_t rpnCommand;
		rpnCommand.cmd_byte = (RF_CONTROL_REMOTE_PORT_NEGOTIATION << 2) | 3;
		rpnCommand.len = (sizeof(dpnParameters) << 1) | 0x01;
		rpnCommand.params.DLCI = (channel->channel_num << 3) | 0x03;
		
		// values copied from lenovo session
		
		rpnCommand.params.baud_rate 		= RF_RPN_BAUD_115200;  // 0x07
		rpnCommand.params.data_stop_parity 	= 0;	// default = 1 stop bit, parity none
		rpnCommand.params.flow_control 		= 0;	// default = no flow control
		rpnCommand.params.xon 				= 0;	// not using xon
		rpnCommand.params.xoff 				= 0;;	// not using xoff
		
		// set a mask telling the remote which of the above
		// parameters are valid .. we set all of them to tell
		// the remote to turn off RTR, RTC 
		
		rpnCommand.params.parameter_mask 	=
			RF_RPN_MASK_BIT_RATE 		|		// 0x0001
			RF_RPN_MASK_DATA_BITS		|		// 0x0002
			RF_RPN_MASK_STOP_BITS		|		// 0x0004
			RF_RPN_MASK_PARITY			|		// 0x0008
			RF_RPN_MASK_PARITY_TYPE 	|		// 0x0010
			
			RF_RPN_MASK_XON_CHAR		|		// 0x0020
			RF_RPN_MASK_XOF_CHAR		|		// 0x0040
			RF_RPN_MASK_X_INPUT			|		// 0x0100t
			RF_RPN_MASK_X_OUTPUT		|		// 0x0200ut
			RF_RPN_MASK_RTR_INPUT		|		// 0x0400
			RF_RPN_MASK_RTR_OUTPUT		|		// 0x0800
			RF_RPN_MASK_RTC_INPUT		|		// 0x1000
			RF_RPN_MASK_RTC_OUTPUT		;		// 0x2000	

		bool is_response = channel->channel_state & RF_CHANNEL_STATE_INCOMING;
		sendPacket(dlc0,!is_response,RF_FRAME_UIH,(u8 *) &rpnCommand,sizeof(rpnCommand));
		channel->channel_state |= RF_CHANNEL_STATE_RPN_COMMAND_SENT;
	}
	else if (!(channel->channel_state & RF_CHANNEL_STATE_RPN_RESPONSE_RECEIVED))
	{
		// do nothing, we are waiting for an RPN response
		// to our RPN command
	}

    
    //------------------------------------
	// OPEN !!!
	//------------------------------------
	// if all those bits are set, channel->channel_num
	// is hereby declared to be OPEN !!
    
    else
    {
		LOG("CHANNEL(%d) opened!!! state=0x%04x",channel->channel_num,channel->channel_state);
		channel->channel_state |= RF_CHANNEL_STATE_OPEN;
    }
    
}	// processChannel()



//---------------------------------
// dlc0 control methods
//---------------------------------

void rfcommLayer::sendSignals(rfChannel *channel)
{
    assert(channel && channel->session);
    LOG("Sending signals(%d)",channel->channel_num);
    rfChannel *dlc0 = findChannel(channel->session,0);
    if (!dlc0) return;
    
    channel->channel_state |= RF_CHANNEL_STATE_SIGNALS_SENT;

    u8 buf[5];
    buf[0] = (RF_CONTROL_MODEM_STATUS << 2) | 3;
    buf[1] = (2 << 1) | 1;
    buf[2] = (channel->channel_num << 3) | 3;
    buf[3] = channel->local.signals | 1;
    buf[4] = channel->local.breaks | 1;         // unused
    
	bool is_response = channel->channel_state & RF_CHANNEL_STATE_INCOMING;
    sendPacket(dlc0,!is_response,RF_FRAME_UIH,buf,4);    
    
}


void rfcommLayer::controlModemStatus(rfChannel *dlc0, bool is_command, u8 *data, u8 len)
    // the length of MSC can be 2 or 3
    // the first byte is the channel number with EA, CR, and direction bits
	//     which we shift out
    // the second byte is signals byte
    // the optional third byte is the break byte
    // the signals byte also has a stupid EA bit, but we don't shift
	//     it out ... we use shifted bitmasks instead ?!?! ...
{
    assert(dlc0 && !dlc0->channel_num);
    // bool is_command = data[0] & 0x02;
    u8 channel_num = data[0] >> 3;
    u8 signal = data[1] & 0xfe;
    u8 breaks = len > 2 ? data[2] & 0xf3 : 0;
    
    assert(dlc0->session);
    rfChannel *channel = findChannel(dlc0->session,channel_num);
    if (!channel) return;
    
    LOG("MSC control for channel(0x%02x=%d) %s %s%s%s%s%s",
        data[0],
        channel_num,
        is_command ? "command" : "response",
        signal & RF_SIGNAL_FC  ? "FC "   : "",
        signal & RF_SIGNAL_RTC ? "RTC "  : "",
        signal & RF_SIGNAL_RTR ? "RTR "  : "",
        signal & RF_SIGNAL_IC  ? "IC "   : "",
        signal & RF_SIGNAL_DV  ? "DV "   : "");
    
    // I guess we just parrot what it said, without the CR bit
    // would have been easier with the whole frame ...
    
    if (is_command) 
    {
        LOG("Signals received(%d)",channel->channel_num);
        channel->channel_state |= RF_CHANNEL_STATE_SIGNALS_RECEIVED;
        
        LOG("Sending signal response(%d)",channel->channel_num);
        channel->channel_state |= RF_CHANNEL_STATE_SIGNAL_RESPONSE_SENT;
	
		// ANALYSIS_CHANGE_NUMBER_2 - clear my bits when I receive a command from the remote
		// which will cause me to send a 2nd signals command and await a 2nd signals response
	
		#if DO_DOUBLE_THE_MSC
			channel->channel_state &= ~(
				RF_CHANNEL_STATE_SIGNALS_SENT |
				RF_CHANNEL_STATE_SIGNAL_RESPONSE_RECEIVED);
		#endif

        u8 buf[5];
        buf[0] = (RF_CONTROL_MODEM_STATUS << 2) | 1;
        buf[1] = (len << 1) | 1;
        buf[2] = (channel_num << 3) | 3;	// uhm, command bit sent it's not INCOMING
        buf[3] = signal | 1;
        buf[4] = breaks | 1;    // unused
        
		// ANALYSIS_CHANGE_NUMBER_1 - I changed the below from 'false' to 'true'
		// as all control packets from us, !STATE_INCOMING, have the outer-level
		// C/R COMMAND bit set 
	
		bool is_response = channel->channel_state & RF_CHANNEL_STATE_INCOMING;
        sendPacket(dlc0,!is_response,RF_FRAME_UIH,buf,len+2);
	    
        channel->remote.signals = signal;
        channel->remote.breaks = breaks;
        
    }
    else
    {
        LOG("Signal response received(%d)",channel->channel_num);
        channel->channel_state |= RF_CHANNEL_STATE_SIGNAL_RESPONSE_RECEIVED;
    }
}



void rfcommLayer::controlParamNegotiation(rfChannel *dlc0, bool is_command, u8 *data, u8 len)
	// the first byte in the PN command is the "DLCI", which is channel
	//    number with ONLY the direction bit (no EA/CR bits), so we shift
	//    it right by 1 to use it ... 
	
{
    assert(dlc0 && !dlc0->channel_num);
	dpnParameters *params = (dpnParameters *) data;

    u8 channel_num = params->DLCI >> 1;
	rfChannel *channel = findChannel(dlc0->session,channel_num);
	
	// this may be the first time we've heard of this channel number
	// so if we don't find it, we add it (for INCOMING connections)
	if (!channel)
		channel = addChannel(dlc0->session,channel_num);
	if (!channel) return;	
	
    LOG("controlParamNegotiation for DLCI(0x%02x)==channel(%d)",params->DLCI,channel_num);
    LOG("    ft(%02x) cl(%02x) priorty(%02x) timer(%02x) mtu(%02x) retrans(%02x) recovery(%02x)",
        params->FrameType,
        params->ConvergenceLayer,
        params->Priority,
        params->ACKTimerTicks,
        params->MaximumFrameSize,
        params->MaxRetransmissions,
        params->RecoveryWindowSize );

    if (!channel_num) return;   // ignore negotiations on dlc0

    if (is_command)
    {
		channel->channel_state |= RF_CHANNEL_STATE_PN_COMMAND_RECEIVED;
		channel->channel_state |= RF_CHANNEL_STATE_PN_RESPONSE_SENT;
		
        //  Save the new channel configuration
        
		channel->priority = params->Priority;
		channel->mtu      = params->MaximumFrameSize;

        LOG("Sending param negotiation response(%d)",channel_num);
        dpnCommand_t dpnResponse;
        dpnResponse.cmd_byte = (RF_CONTROL_DLC_PARAMETER_NEGOTIATION << 2) | 1;
            // | 1 for response, | 3 for command
        dpnResponse.len = (sizeof(dpnParameters) << 1) | 0x01;
        memcpy(&dpnResponse.params, params, sizeof(dpnParameters));
        dpnResponse.params.ConvergenceLayer = 0x00;
        dpnResponse.params.FrameType &= 0xe0;
			// to match lenovo
			
		bool is_response = channel->channel_state & RF_CHANNEL_STATE_INCOMING;
        sendPacket(dlc0,!is_response,RF_FRAME_UIH,(u8 *) &dpnResponse,sizeof(dpnResponse));
    }
	else
	{
		// set the bit that it was received
		
		channel->channel_state |= RF_CHANNEL_STATE_PN_RESPONSE_RECEIVED;
	}
}




void rfcommLayer::controlRemotePortNegotiation(rfChannel *dlc0, bool is_command, u8 *data, u8 len)
	// The length byte in an RPN command is either 1 or 8.
	// If the length is 1, then there is a single value byte which contains
	// the DLCI for the connection, and the message is interpreted
	// as a request for the link’s parameters.
	//
	// In this case, the remote end replies with the current parameters
	// on the link.
	//
	// If the length byte is set to 8, then eight bytes of link parameters
	// follow. If they are sent in a command, then they are a request to
	// set up the link’s parameters.
{
    assert(dlc0 && !dlc0->channel_num);
	rpnParameters *params = (rpnParameters *) data;
    u8 channel_num = params->DLCI >> 3;
	
	rfChannel *channel = findChannel(dlc0->session,channel_num);
	if (!channel) return;	
	
    LOG("controlRemotePortNegotiation for DLCI(0x%02x)==channel(%d)",params->DLCI,channel_num);
    LOG("    baud(%02x) dsp(%02x) flow(%02x) xon(%02x) xoff(%02x) mask(%04x)",
        params->baud_rate,
        params->data_stop_parity,
        params->flow_control,
        params->xon,
        params->xoff,
        params->parameter_mask);

	// we accept whatever they send
	// may be order of operations dependent with our state machine
	
    if (is_command)
    {
		// set both bits
		
		channel->channel_state |= RF_CHANNEL_STATE_RPN_COMMAND_RECEIVED;
		channel->channel_state |= RF_CHANNEL_STATE_RPN_RESPONSE_SENT;
        LOG("Sending remote param negotiation response(%d)",channel_num);

		// 1 for inner level response
		// outer level response bit depends on INCOMING
		
        rpnCommand_t rpnResponse;
        rpnResponse.cmd_byte = (RF_CONTROL_REMOTE_PORT_NEGOTIATION << 2) | 1;
        rpnResponse.len = (sizeof(dpnParameters) << 1) | 0x01;
        memcpy(&rpnResponse.params, params, sizeof(rpnParameters));
		bool is_response = channel->channel_state & RF_CHANNEL_STATE_INCOMING;
        sendPacket(dlc0,!is_response,RF_FRAME_UIH,(u8 *) &rpnResponse,sizeof(rpnResponse));
    }
	else	// set the response bit
	{
		channel->channel_state |= RF_CHANNEL_STATE_RPN_RESPONSE_RECEIVED;
	}
}





//--------------------------------------------
// client API
//--------------------------------------------


void rfcommLayer::sendData(rfChannel *channel, const u8 *data, u16 len)
{
    assert(channel && channel->session);
    if (!channel->channel_num)
    {
        LOG_ERROR("illegal attempt to sendData() on dlc0",0);
        return;
    }
    if (!(channel->channel_state & RF_CHANNEL_STATE_OPEN))
    {
        LOG_ERROR("attempt to sendData() on unopened channel",0);
        return;
    }
    sendPacket(channel,true,RF_FRAME_UIH,(u8*)data,len);    
}



void rfcommLayer::closeRFChannel(rfChannel *channel)
{
    // supposed to issue a DM request
    // and wait for a DSC reply, but we're
    // just gonna cave the whole session
    
    assert(channel);
    assert(channel->session);
    assert(channel->session->lcn);
    if (channel &&
        channel->session &&
        channel->session->lcn)
    {
        m_pLCAP->closeConnection(channel->session->lcn);
        deleteChannel(channel);
    }
}



rfChannel *rfcommLayer::openRFChannel(u8 *addr, u8 channel_num)
    // client must know, and pass in desired channel_num (from SDP)
{
    LOG("openRFChannel(%s,%d)",addrToString(addr),channel_num);
    
    // see if there's already an rfSession to device at addr
    
    rfSession *session = 0;
    for (u16 i=0; i<RF_MAX_SESSIONS; i++)
    {
        rfSession *ses = &m_sessions[i];
        if (ses->lcn &&
            ses->lcn->device &&
            !memcmp(ses->lcn->device->addr,addr,BT_ADDR_SIZE))
        {
            session = ses;
            break;
        }
    }
    
    // no existing session, so we have to start an lcap connection
    
    if (!session)
    {
        lcapConnection *lcn = m_pLCAP->startConnection(addr, RFCOMM_PSM);
        if (!lcn) return 0;
        session = addSession(lcn);
        if (!session) return 0;
        
        // add channel 0
        
        rfChannel *channel = addChannel(session,0);
        if (!channel) return 0;
    }
    
    // for now I am going to assume that there is only one
    // channel per destination service ... i.e. a channel
    // can only be opened once.
    
    else
    {
        for (u16 i=0; i<RF_MAX_CHANNELS; i++)
        {
            if (session->channel[i].session &&
                session->channel[i].channel_num == channel_num)
            {
                LOG_ERROR("ERROR - channel(%d) is already open in session(0x%04x)",
                    channel_num,
                    session->lcn->local_cid);
                return 0;
            }
        }
    }
    
    rfChannel *channel = addChannel(session,channel_num);
    
    return channel;
}



int rfcommLayer::getOpenChannels(rfChannel **buf, int max)
{
    int num = 0;
    assert(max);
    for (u16 i=0; i<RF_MAX_SESSIONS; i++)
    {
        rfSession *session = &m_sessions[i];
        if (session->lcn)
        {
            for (u16 j=0; j<RF_MAX_CHANNELS; j++)
            {
                rfChannel *channel = &session->channel[j];
                if (channel->session &&
                    (channel->channel_state & RF_CHANNEL_STATE_OPEN) &&
		    !(channel->channel_state & RF_CHANNEL_STATE_CLOSING) &&
		    !(channel->channel_state & RF_CHANNEL_STATE_CLOSED))
                {
                    num++;
                    *buf++ = channel;
                    if (!--max)
                        break;
                }
            }
        }
    }
    return num;
}

