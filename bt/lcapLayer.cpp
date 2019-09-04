#include "_hci_defs.h"
#include "lcapLayer.h"
#include <circle/util.h>
#include <circle/sched/scheduler.h>

#define LCAP_CID	0x0001
	// probably defined else where, this is the
	// main CAP signal channel id

static const char *log_name = "lcap";

#define SHOW_INCOMING_DATA		0
#define SHOW_OUTGOING_DATA      0

#define CLOSE_UNUSED_HCI_HANDLES    1
    // if we close all the lcap channels to an hci_handle
    // we will also close the HCI handle 


// It appears to be required that you (a) respond to any config and
// info requests, and that (b) you MUST send a config request for each
// new lcap channel.  It took me 2 hard weeks to get through to a remote
// SDP the first time.  The only option I left was maybe to do a pending
// connection, in case I find some other cranky remote needs it.

#define DO_CONFIG_REQUESTS  	 	1		// REQUIRED
#define SEND_PENDING_CONNECTION  	0
#define WAIT_FOR_CONFIG_RESPONSES 	1
	// I found it works best if we wait for a config request before
	// we send our own config request, and that we wait, even after
	// EVENT_CONNECTED completed, to call the HCI connection "open"
	// and let clients start using the port.

// lcap connection state bits

#define LCAP_STATE_CONNECTING                   0x0001
#define LCAP_STATE_CONNECTED                    0x0002
#define LCAP_STATE_ERROR                        0x0008

#define LCAP_STATE_CONFIG_REQUEST_RECEIVED      0x0010
#define LCAP_STATE_CONFIG_RESPONSE_SENT         0x0020
#define LCAP_STATE_CONFIG_REQUEST_SENT          0x0040
#define LCAP_STATE_CONFIG_RESPONSE_RECEIVED     0x0080


// Forwards of static packet filling methods

u16 infoResponse(u8 *buf, lcap_info_request *p);


//-------------------------------------------------
// ctor and dtor
//-------------------------------------------------

lcapLayer::lcapLayer(hciLayer *pHCILayer) :
	m_pHCI(pHCILayer)
{
	m_num_clients = 0;
	memset(m_pClients,0,MAX_LCAP_CLIENTS * sizeof(void *));
	memset(m_connections,0,MAX_LCAP_CONNECTIONS * sizeof(lcapConnection));
	m_pHCI->registerAsClient(this);
}


lcapLayer::~lcapLayer (void)
{
	m_pHCI->registerAsClient(0);
	memset(m_connections,0,MAX_LCAP_CONNECTIONS * sizeof(lcapConnection));
	memset(m_pClients,0,MAX_LCAP_CLIENTS * sizeof(void *));
	m_num_clients = 0;
}



void lcapLayer::sendData(const void *buffer, unsigned length)
{
	lcap_data_packet_header *hdr = (lcap_data_packet_header *) buffer;
	hdr->lcap_d.len = length - sizeof(lcap_data_packet_header);

	#if SHOW_OUTGOING_DATA
		display_bytes("<lcap",(u8 *)buffer,length);
	#endif

	m_pHCI->sendData(buffer,length);
}





//----------------------------------------
// Utilities
//-----------------------------------------

lcapClient *lcapLayer::findClientByPSM(u16 psm)
{
	for (u16 i=0; i<m_num_clients; i++)
	{
		lcapClient *pClient = m_pClients[i];
		if (pClient->getPSM() == psm)
		{
			return pClient;
		}
	}
	return 0;	
}


void lcapLayer::deleteConnection(lcapConnection *lcn, bool ifLastCloseHCI)
{
	assert(lcn);
	LOG_LCAP("deleteConnection(%s:0x%04x) lcid=0x%04x rcid=0x%04x",
		lcn->device ? addrToString(lcn->device->addr) : "unknown",
		lcn->device ? lcn->device->handle : 0xFFFF,
		lcn->local_cid,
		lcn->remote_cid);
	
	#if CLOSE_UNUSED_HCI_HANDLES
		if (ifLastCloseHCI &&
			lcn->device &&
			lcn->device->handle)
		{
			bool any = false;
			for (u16 i=0; i<MAX_LCAP_CONNECTIONS; i++)
			{
				lcapConnection *l = &m_connections[i];
				if (l != lcn &&
					l->device == lcn->device)
				{
					any = true;
					break;
				}
			}
			
			if (!any)
			{
				LOG_LCAP("last lcap connection to (%s:0x%04x).  Closing hci connection.",
					addrToString(lcn->device->addr),
					lcn->device->handle);
				m_pHCI->closeConnection(lcn->device);
			}
		}
	#endif

	memset(lcn,0,sizeof(lcapConnection));
}


lcapConnection *lcapLayer::addConnection(hciRemoteDevice *device, u16 psm, u16 rcid)
{
	assert(device);
	LOG_LCAP("addConnection(%s:0x%04x) psm=0x%04x lcid=0x%04x rcid=0x%04x",
		addrToString(device->addr),
		device->handle,
		psm,
		device->next_lcap_cid,
		rcid);
	
	for (u16 i=0; i<MAX_LCAP_CONNECTIONS; i++)
	{
		lcapConnection *lcn = &m_connections[i];
		if (!lcn->local_cid)
		{
			lcn->psm = psm;
			lcn->local_cid = device->next_lcap_cid++;
			lcn->device = device;
			lcn->lcap_state = 0;
			lcn->remote_cid = rcid;
			lcn->pClient = findClientByPSM(lcn->psm);
				// hmm .. incoming or outgoing?
				// we associate the psm with any matching service 
				
			LOG_LCAP("addConnection(%s:0x%04x,0x%04x) added lcid(0x%04x) client=0x%08x",
				addrToString(device->addr),
				device->handle,
				rcid,
				lcn->local_cid,
				lcn->pClient);
			return lcn;
		}
	}
	LOG_ERROR("could not addConnection(%s:0x%04x) psm=0x%04x lcid=0x%04x rcid=0x%04x - no room!!",
		addrToString(device->addr),
		device->handle,
		psm,
		device->next_lcap_cid,
		rcid);
	return 0;
}




lcapConnection *lcapLayer::findRemoteCid(hciRemoteDevice *device, u16 cid)
	// used to make sure it does NOT already exist in a new
	// connection request
{
	assert(device);
	for (u16 i=0; i<MAX_LCAP_CONNECTIONS; i++)
	{
		lcapConnection *lcn = &m_connections[i];
		if (lcn->device == device &&
			lcn->remote_cid == cid)
			return lcn;
	}
	return 0;
}



lcapConnection *lcapLayer::findLocalCid(u16 hci_handle, u16 cid)
{
	hci_handle &= 0x0fff;
	hciRemoteDevice *device = m_pHCI->findDeviceByHandle(hci_handle);
	if (!device)
	{
		LOG_ERROR("Could not find device for hci_handle(0x%04x)",hci_handle);
		return 0;
	}
	for (u16 i=0; i<MAX_LCAP_CONNECTIONS; i++)
	{
		lcapConnection *lcn = &m_connections[i];
		if (lcn->device == device &&
			lcn->local_cid == cid)
			return lcn;
	}
	LOG_ERROR("Could not find lcn for hci_handle(0x%04x) and lcid(0x%04x)",
		hci_handle,
		cid);
	return 0;
}
	




	

const char *lcapEventName(u16 event)
{
	if (event == LCAP_EVENT_CONNECTING    ) return "LCAP_EVENT_CONNECTING";
	if (event == LCAP_EVENT_CONNECTED     ) return "LCAP_EVENT_CONNECTED";
	if (event == LCAP_EVENT_DISCONNECTED  ) return "LCAP_EVENT_DISCONNECTED";
	if (event == LCAP_EVENT_DATA          ) return "LCAP_EVENT_DATA";
	if (event == LCAP_EVENT_ERROR         ) return "LCAP_EVENT_ERROR";
	return "unknown lcapEvent";
}


const char *hccEventName(u16 event)
{
	if (event == HCC_EVENT_CONNECTED        ) return "HCC_EVENT_CONNECTED";
	if (event == HCC_EVENT_DISCONNECTED     ) return "HCC_EVENT_DISCONNECTED";
	if (event == HCC_EVENT_CONNECTION_ERROR ) return "HCC_EVENT_CONNECTION_ERROR";
	if (event == HCC_INQUIRY_COMPLETE       ) return "HCC_INQUIRY_COMPLETE";
	if (event == HCC_INQUIRY_DEVICE_FOUND   ) return "HCC_INQUIRY_DEVICE_FOUND";
	if (event == HCC_INQUIRY_NAME_FOUND     ) return "HCC_INQUIRY_NAME_FOUND";
	return "unknown hccEvent";
}


const char *getLcapCommandString(u8 b)
{
	if (b == 0x01)  return "LCAP_COMMAND_REJECT";
	if (b == 0x02)  return "LCAP_CONNECTION_REQUEST";
	if (b == 0x03)  return "LCAP_CONNECTION_RESPONSE";
	if (b == 0x04)  return "LCAP_CONFIGURE_REQUEST";
	if (b == 0x05)  return "LCAP_CONFIGURE_RESPONSE";
	if (b == 0x06)  return "LCAP_DISCONNECTION_REQUEST";
	if (b == 0x07)  return "LCAP_DISCONNECTION_RESPONSE";
	if (b == 0x08)  return "LCAP_ECHO_REQUEST";
	if (b == 0x09)  return "LCAP_ECHO_RESPONSE";
	if (b == 0x0a)  return "LCAP_INFORMATION_REQUEST";
	if (b == 0x0b)  return "LCAP_INFORMATION_RESPONSE";
	if (b == 0x12)  return "LCAP_PARAMETER_UPDATE_REQUEST";
	if (b == 0x13)  return "LCAP_PARAMETER_UPDATE_RESPONSE";
	if (b == 0x14)  return "LCAP_LE_CONNECTION_REQUEST";
	if (b == 0x15)  return "LCAP_LE_CONNECTION_RESPONSE";
	if (b == 0x16)  return "LCAP_LE_FLOW_CONTROL_CREDIT";
	return "UNKNOWN LCAP COMMAND";
}


	

//----------------------------------------------------------
// Receive Events from HCI and dispatch them to clients
//----------------------------------------------------------

void lcapLayer::dispatchEvents(hciRemoteDevice *device, u16 event_type)
	// send events to client layers
	// we call deleteConnection(false) because we know we
	// are already in an hciCloseConnection cycle 
{
	assert(device);
	for (u16 i=0; i<MAX_LCAP_CONNECTIONS; i++)
	{
		lcapConnection *lcn = &m_connections[i];
		if (lcn->device == device && lcn->pClient)
		{
			if (event_type == LCAP_EVENT_ERROR)
				lcn->lcap_state |= LCAP_STATE_ERROR;
			lcn->pClient->lcapEvent(event_type,lcn,0,0);
			if (event_type == LCAP_EVENT_DISCONNECTED)
				deleteConnection(lcn,false);
		}
	}
}


void lcapLayer::receiveEvent(u16 event, hciRemoteDevice *device)
	// receive event from HCI layer
{
	if (event != HCC_INQUIRY_DEVICE_FOUND)
		PRINT_LCAP("lcap: %s(0x%04x) handle(0x%04x) %s %s\n",
			hccEventName(event),
			event,
			device ? device->handle : 0,
			device ? (char *) addrToString(device->addr) : "",
			device ? device->name : "");

	switch (event)
	{
		case HCC_EVENT_CONNECTED: // start any LCAP connections that were pending
		{
			assert(device);
			for	(int i=0; i<MAX_LCAP_CONNECTIONS; i++)
			{
				lcapConnection *lcn = &m_connections[i];
				if (lcn->device == device &&
					lcn->lcap_state & LCAP_EVENT_CONNECTING)
				{
					LOG_LCAP("Starting pending connection to %s(%s):0x%04x for lcid(0x%04x) psm(0x%04x)",
						addrToString(device->addr),
						device->name,
						device->handle,
						lcn->local_cid,
						lcn->psm);
					
					lcap_connect_request cmd;
					u16 hci_handle = device->handle & 0x0fff;
					
					cmd.hdr.hci.handle = hci_handle | 0x2000;
					cmd.hdr.lcap_d.cid = LCAP_CID;
					cmd.hdr.lcap_c.cmd = LCAP_CONNECTION_REQUEST;
					cmd.hdr.lcap_c.id = device->next_lcap_id++;
					cmd.hdr.lcap_c.cmd_len = 4;
					cmd.psm = lcn->psm;
					cmd.src_cid = lcn->local_cid;
			
					PRINT_LCAP("<-- LCAP_CONNECTION_REQUEST[%d] lcid=0x%04x\n",
						cmd.hdr.lcap_c.id,
						lcn->local_cid);
					sendData(&cmd,sizeof(cmd));
				}
			}
			break;
		}
		
		case HCC_EVENT_CONNECTION_ERROR:
		{
			assert(device);
			dispatchEvents(device,LCAP_EVENT_ERROR);
			break;
		}
		case HCC_EVENT_DISCONNECTED:
		{
			assert(device);
			dispatchEvents(device,LCAP_EVENT_DISCONNECTED);
			break;
		}
	}
}




//===========================================================================
// LCAP State Machine and Data Packet event handler
//===========================================================================

void lcapLayer::checkLcapConnectionState(lcap_command_packet_header *req, lcapConnection *lcn)
{
	
	#if DO_CONFIG_REQUESTS
		assert(lcn);
		if (lcn->remote_cid &&
			lcn->lcap_state & LCAP_STATE_CONFIG_REQUEST_RECEIVED &&
			!(lcn->lcap_state & LCAP_STATE_CONFIG_REQUEST_SENT))
		{
			lcn->lcap_state |= LCAP_STATE_CONFIG_REQUEST_SENT;
			
			assert(lcn->device);
			
			lcap_config_request cmd;
			memset(&cmd,0,sizeof(cmd));
			cmd.hdr.hci.handle = lcn->device->handle | 0x2000;
			cmd.hdr.lcap_d.cid = LCAP_CID;
			cmd.hdr.lcap_c.cmd = LCAP_CONFIGURE_REQUEST;
			cmd.hdr.lcap_c.id = lcn->device->next_lcap_id++;
			cmd.hdr.lcap_c.cmd_len = 8;
			cmd.dest_cid = lcn->remote_cid;
			cmd.flags = 0;
			cmd.options[0] = 1;			// type MTU, not optional
			cmd.options[1] = 2;         // len
			cmd.options[2] = 0x00;		// our MTU LSB
			cmd.options[3] = 0x02;		// our MTU MSB

			PRINT_LCAP("<-- LCAP_CONFIG_REQUEST[%d]\n",cmd.hdr.lcap_c.id);
			sendData(&cmd,sizeof(cmd));
		}
	#endif

	#if SEND_PENDING_CONNECTION || WAIT_FOR_CONFIG_RESPONSES
		assert(lcn);
		if ((lcn->lcap_state & LCAP_STATE_CONFIG_REQUEST_RECEIVED) &&
			(lcn->lcap_state & LCAP_STATE_CONFIG_RESPONSE_SENT) &&
			(lcn->lcap_state & LCAP_STATE_CONFIG_REQUEST_SENT) &&
			(lcn->lcap_state & LCAP_STATE_CONFIG_RESPONSE_RECEIVED) &&
			lcn->local_cid )
		{
			#if SEND_PENDING_CONNECTION
				lcap_connect_response cmd;
				memset(&cmd,0,sizeof(cmd));
				
				u16 hci_handle = req->hdr.hci.handle & 0x0fff;
				u16 rcid = ;
				
				cmd.hdr.hci.handle 		= hci_handle | 0x2000;		// word - handle or'd with flags
				cmd.hdr.hci.len 		= 0;						// word - hci length (filled in by hci::sendData())
				cmd.hdr.lcap_d.len		= 0;						// word - lcap length (filled in by lcap::sendData())
				cmd.hdr.lcap_d.cid		= 0x0001;					// word - main channel
				cmd.hdr.lcap_c.cmd   	= LCAP_CONNECTION_RESPONSE;	// byte - lcap command
				cmd.hdr.lcap_c.id 		= req->hdr.lcap_c.id;		// byte - return the unique id from requestor
				cmd.hdr.lcap_c.cmd_len 	= 8;						// word - length of following
				cmd.dest_cid			= lcn->local_cid;			// word - the local cid we are granting
				cmd.src_cid 			= lcn->remote_cid;			// word - the remote cid being associated with the local cid
				cmd.result 				= 0;						// word - 0=complete, 1=pending
				cmd.status 				= 0;						// word - 0=nothing to say

				PRINT_LCAP("<-- LCAP_CONNECTION_RESPONSE[%d] 0=completed\n",cmd.hdr.lcap_c.id);
				sendData(&cmd,sizeof(cmd));
			#endif
			
			#if WAIT_FOR_CONFIG_RESPONSES
				lcapClient *pClient = findClientByPSM(lcn->psm);
				if (pClient)
				{
					pClient->lcapEvent(LCAP_EVENT_CONNECTED,lcn,0,0);
				}
			#endif
			
		}
	#endif
}
			



void lcapLayer::receiveData(const void *buffer, unsigned length)
{
	#if SHOW_INCOMING_DATA
		display_bytes("lcap>",(u8 *)buffer,length);
	#endif
	
	lcap_command_packet_header *hdr = (lcap_command_packet_header *) buffer;
	
	//-----------------------
	// client data received
	//-----------------------
	
	if (hdr->lcap_d.cid != LCAP_CID)
	{
		u16 hci_handle =  hdr->hci.handle & 0x0fff;
		u16 lcid =  hdr->lcap_d.cid;
		
		PRINT_LCAP("--> LCAP CLIENT PACKET hci_handle(0x%04x) cid(0x%04x)\n", hci_handle,lcid);
		lcapConnection *lcn = findLocalCid(hci_handle,lcid);
		if (!lcn) return;

		// we should only receive data for a client
		// specifically after a channel has been opened for that client,
		// and thus we have set the client pointer ..
		
		assert(lcn->pClient);
		if (lcn->pClient)
		{
			// send the data after the hci and lcap headers
			
			LOG_LCAP("Sending %d data bytes to client %s for psm(0x%04x) and lcid(0x%04x)",
				hdr->lcap_d.len, lcn->pClient->serviceName(), lcn->psm, lcid);
			u8 *data_ptr = &((u8 *)buffer)[sizeof(lcap_data_packet_header)];
			
			lcn->pClient->lcapEvent(LCAP_EVENT_DATA, lcn, data_ptr, hdr->lcap_d.len);
			return;
		}
		LOG_ERROR("LCAP packet on hci_handle(0x%04x) to unknown psm(0x%04x)\n", hci_handle,lcn->psm);
		return;
	}

	
	//--------------------------
	// lcap event handling
	//--------------------------

	switch (hdr->lcap_c.cmd)
	{
		case LCAP_CONNECTION_REQUEST:	// 0x02
		{
			lcap_connect_request *req = (lcap_connect_request *) buffer;
			u16 hci_handle =  req->hdr.hci.handle & 0x0fff;
			u16 rcid = req->src_cid;
			PRINT_LCAP("--> LCAP_CONNECTION_REQUEST[%d] hci_handle(0x%04x) PSM(0x%04x) rcid=0x%04x\n",
				hci_handle,
				req->hdr.lcap_c.id,
				req->psm,
				rcid);

			// prh we should reject the request if it's an unknown PSM
			
			hciRemoteDevice *device = m_pHCI->findDeviceByHandle(hci_handle);
			if (!device)
			{
				LOG_ERROR("LCAP connection request from unknown hci_handle(0x%04x)\n", hci_handle);
				return;
			}
			
			// we should not receive multiple connection requests
			// or requests to connect to a channel that is already open
		
			lcapConnection *lcn = findRemoteCid(device,rcid);
			if (lcn)
			{
				LOG_ERROR("LCAP connection request to existing hci_handle(0x%04x) rcid(0x%04x)\n",
					hci_handle,
					rcid);
				return;
			}
			
			// create a new connection
			
			lcn = addConnection(device,req->psm,rcid);
			if (!lcn) return;

			// send the connection response
			
			lcap_connect_response cmd;
			memset(&cmd,0,sizeof(cmd));
			
			cmd.hdr.hci.handle 		= hci_handle | 0x2000;		// word - handle or'd with flags
			cmd.hdr.lcap_d.cid		= 0x0001;					// word - main channel
			cmd.hdr.lcap_c.cmd   	= LCAP_CONNECTION_RESPONSE;	// byte - lcap command
			cmd.hdr.lcap_c.id 		= req->hdr.lcap_c.id;		// byte - return the unique id from requestor
			cmd.hdr.lcap_c.cmd_len 	= 8;						// word - length of following
			cmd.dest_cid			= lcn->local_cid;			// word - the local cid we are granting
			cmd.src_cid 			= lcn->remote_cid;			// word - the remote cid being associated with the local cid
			#if SEND_PENDING_CONNECTION
				cmd.result 			= 1;						// word - 0=complete, 1=pending
			#else
				cmd.result 			= 0;						// word - 0=complete, 1=pending
			#endif				
			cmd.status 				= 0;						// word - 0=nothing to say

			PRINT_LCAP("<-- LCAP_CONNECTION_RESPONSE[%d] %d=%s\n",
				   cmd.hdr.lcap_c.id,
				   cmd.result,
				   cmd.result ? "pending" : "completed");
			sendData(&cmd,sizeof(cmd));
				
			checkLcapConnectionState((lcap_command_packet_header *) req,lcn);
			break;
		}
	
		case LCAP_CONNECTION_RESPONSE:	// 0x03
		{
			lcap_connect_response *rsp = (lcap_connect_response *) buffer;
			
			PRINT_LCAP("--> LCAP_CONNECTION_RESPONSE[%d] dest_cid=0x%04x  src_cid=0x%04x rslt(%d) stat(%d)\n",
				rsp->hdr.lcap_c.id,
				rsp->dest_cid,
				rsp->src_cid,
				rsp->result,
				rsp->status);

			switch (rsp->result)
			{
				case 0:
					PRINT_LCAP("    Connection succesful\n",0);
					break;
				case 1:
					PRINT_LCAP("    Connection pending ...",0);
					switch (rsp->status)
					{
						case 0:
							PRINT_LCAP("0 No further information available\n",0);
							break;
						case 1:
							PRINT_LCAP("1 Authentication pending\n",0);
							break;
						case 2:
							PRINT_LCAP("22 Authorization pending\n",0);
							break;
						default:
							PRINT_LCAP("UNKNOWN STATUS(%d)\n",rsp->status,0);
							break;
					}
					break;
				case 2:
					PRINT_LCAP("    Connection refused – PSM not supported\n",0);
					return;
				case 3:
					PRINT_LCAP("    Connection refused – security block\n",0);
					return;
				case 4:
					PRINT_LCAP("    Connection refused – no resources available\n",0);
					return;
				default:
					LOG_ERROR("    UNKNOWN CONNECTION RESULT!!!",0);
					return;
			}

			u16 hci_handle = rsp->hdr.hci.handle;
			u16 rcid = rsp->dest_cid;
			u16 lcid = rsp->src_cid;
			
			lcapConnection *lcn = findLocalCid(hci_handle,lcid);
			if (!lcn) return;
			assert(lcn->remote_cid == 0 || lcn->remote_cid == rcid);

			lcapClient *pClient = lcn->pClient;	
			
			if (rsp->result == 0 ||
				rsp->result == 1 )
			{
				// add the remote cid
				
				lcn->remote_cid = rcid;
				if (rsp->result == 0)
				{
					lcn->lcap_state |= LCAP_STATE_CONNECTED;
					
					#if !WAIT_FOR_CONFIG_RESPONSES
						if (pClient)
							pClient->lcapEvent(LCAP_EVENT_CONNECTED,lcn,0,0);
					#endif
				}
				
				checkLcapConnectionState((lcap_command_packet_header *)rsp,lcn);
			}
			else	// connection refused/rejected
			{
				if (pClient)
					pClient->lcapEvent(LCAP_EVENT_ERROR,lcn,0,0);
				deleteConnection(lcn,true);
			}

			break;
		}

		case LCAP_CONFIGURE_REQUEST: // 0x04
		{
			lcap_config_request *req = (lcap_config_request *) buffer;
			u16 hci_handle =  req->hdr.hci.handle;
			u16 lcid 	   =  req->dest_cid;
			u8 opt_bytes   = (req->hdr.lcap_c.cmd_len - 4);

			PRINT_LCAP("--> LCAP_CONFIG_REQUEST[%d] hci_handle(0x%04x) lcid=0x%04x opt_bytes(%d) flag(0x%04x)\n",
				req->hdr.lcap_c.id,
				hci_handle,
				lcid,
				opt_bytes,
				req->flags);
			assert(req->flags == 0);
			
			// the options are themselves a stream
			
			int i = 0;
			u8 *o = (u8 *) &req->options;
			while (i < opt_bytes)
			{
				u8 type = *o++;  i++;		// a byte for the type
				// u8 optional = type & 0x80;	// high order bit of the type says if it's an optional parameter
				type &= 0x7f;				// we only get, and respond to, MANDATORY ones
				
				u8 len  = *o++;  i++;		// a byte for the len that *could* have a high order continuation bit itself
				
				PRINT_LCAP(type & 0x80 ? "    optional " : "    MANDATORY ",0);
				switch (type)
				{
					case 1 : PRINT_LCAP("MTU 0x%04x\n",*(u16 *) o); break;
					case 2 : PRINT_LCAP("FLUSH TIMEOUT 0x%04x\n",*(u16 *) o);  break;
					case 3 : display_bytes("QOS",o,len); break;
					default : PRINT_LCAP("UNKNOWN OPTION\n",0);
				}
				i += len;
				o += len;
			}

			lcapConnection *lcn = findLocalCid(hci_handle,lcid);
			if (!lcn) return;
			lcn->lcap_state |= LCAP_STATE_CONFIG_REQUEST_RECEIVED;

			// create a config response and send it

			lcap_config_response cmd;
			memset(&cmd,0,sizeof(cmd));
			cmd.hdr.hci.handle 		= hci_handle | 0x2000;		
			cmd.hdr.lcap_d.cid		= 0x0001;					
			cmd.hdr.lcap_c.cmd   	= LCAP_CONFIGURE_RESPONSE;	
			cmd.hdr.lcap_c.id 		= req->hdr.lcap_c.id;		
			cmd.hdr.lcap_c.cmd_len 	= 6;						
			cmd.dest_cid 			= lcn->remote_cid;	// word - the remote cid being associated with the local cid
			cmd.result 				= 0;
			cmd.flags 				= 0;

			PRINT_LCAP("<-- LCAP_CONFIG_RESPONSE[%d]\n",cmd.hdr.lcap_c.id);
			sendData(&cmd,sizeof(cmd));

			lcn->lcap_state |= LCAP_STATE_CONFIG_RESPONSE_SENT;
			checkLcapConnectionState((lcap_command_packet_header *)req,lcn);
			break; 
		}

		case LCAP_CONFIGURE_RESPONSE: // 0x05
		{
			lcap_config_response *rsp = (lcap_config_response *) buffer;
			u16 hci_handle = rsp->hdr.hci.handle;
			u16 lcid 	   = rsp->dest_cid;		 // local cid
			
			assert(!rsp->result);
			assert(!rsp->flags);
			
			PRINT_LCAP("--> LCAP_CONFIG_RESPONSE[%d] hci_handle(0x%04x) lcid=0x%04x\n",
				rsp->hdr.lcap_c.id,
				hci_handle,
				lcid);

			lcapConnection *lcn = findLocalCid(hci_handle,lcid);
			if (!lcn) return;
			lcn->lcap_state |= LCAP_STATE_CONFIG_RESPONSE_RECEIVED;

			checkLcapConnectionState((lcap_command_packet_header *)rsp,lcn);
			break;
		}
		
		
		case LCAP_INFORMATION_REQUEST : // 0x0a
		{
			lcap_info_request *req = (lcap_info_request *) buffer;
			u16 type = req->info_type;
			
			PRINT_LCAP("--> LCAP_INFORMATION_REQUEST[%d] hci_handle(0x%04x) info_type(0x%04x)=%s\n",
				req->hdr.lcap_c.id,
				req->hdr.hci.handle,
				type,
				type == 1 ? "connectionless MTU" :
				type == 2 ? "extended features supported" :
				type == 3 ? "fixed channels supported" :
				"unknown");
			u16 hci_handle = req->hdr.hci.handle & 0x0fff;

			if (type == 2)
			{
				lcap_info_response_features cmd;
				memset(&cmd,0,sizeof(cmd));

				cmd.hdr.hci.handle 		= hci_handle | 0x2000;		
				cmd.hdr.lcap_d.cid		= 0x0001;					
				cmd.hdr.lcap_c.cmd   	= LCAP_INFORMATION_RESPONSE;
				cmd.hdr.lcap_c.id 		= req->hdr.lcap_c.id;		
				cmd.hdr.lcap_c.cmd_len 	= 8;						

				cmd.info_type 	= type;
				cmd.result 		= 0;
				cmd.data[0] 	= LCAP_FEATURE_FIXED_CHANNELS;
				cmd.data[1]		= 0;
				cmd.data[2]		= 0;
				cmd.data[3]		= 0;
	
				PRINT_LCAP("<-- LCAP_INFO_RESPONSE[%d]\n",cmd.hdr.lcap_c.id);
				sendData(&cmd,sizeof(cmd));
			}
			else if (type == 3)
			{
				lcap_info_response_fixed_channels cmd;
				memset(&cmd,0,sizeof(cmd));
				
				cmd.hdr.hci.handle 		= hci_handle | 0x2000;		
				cmd.hdr.lcap_d.cid		= 0x0001;					
				cmd.hdr.lcap_c.cmd   	= LCAP_INFORMATION_RESPONSE;
				cmd.hdr.lcap_c.id 		= req->hdr.lcap_c.id;		
				cmd.hdr.lcap_c.cmd_len 	= 12;						

				cmd.info_type 	= type;
				cmd.result 		= 0;
				cmd.data[0] 	= 0x02;		// // bit 2 is the l2cap fixed channel
				cmd.data[1]		= 0;
				cmd.data[2]		= 0;
				cmd.data[3]		= 0;
				cmd.data[4]		= 0;
				cmd.data[5]		= 0;
				cmd.data[6]		= 0;
				cmd.data[7]		= 0;
	
				PRINT_LCAP("<-- LCAP_INFO_RESPONSE[%d]\n",cmd.hdr.lcap_c.id);
				sendData(&cmd,sizeof(cmd));
			}
			else
			{
				PRINT_LCAP("--> UNHANDLED INFORMATION REQUEST TYPE(%d)\n",type);
			}
			break;
		}
		
		case LCAP_COMMAND_REJECT :
		{
			lcap_command_reject *reject = (lcap_command_reject *) buffer;
			u16 reason = reject->reason;
			PRINT_LCAP("--> LCAP_COMMAND_REJECT[%d] hci_handle(0x%04x) reason=0x%04x %s\n",
				reject->hdr.lcap_c.id,
				reject->hdr.hci.handle,
				reason,
				reason == 0 ? "command not understood" :
				reason == 1 ? "mtu exceeded" :
				reason == 2 ? "invalid cid in request" :
				"other" );
			if (reason == 1)
				PRINT_LCAP("    max_mtu=%d\n",reject->max_mtu);
			else if (reason == 2)
				PRINT_LCAP("    lcid=0x%04x    rcid=0x%04x\n",reject->lcid,reject->rcid);
			break;
		}
		
		case LCAP_DISCONNECTION_REQUEST	:
		{
			lcap_disconnect_request	*req = (lcap_disconnect_request *) buffer;	
			PRINT_LCAP("--> LCAP_DISCONNECTION_REQUEST[%d]  hci_handle(0x%04x) dest_cid=0x%04x src_cid=0x%04x\n",
				req->hdr.lcap_c.id,
				req->hdr.hci.handle,
				req->src_cid,
				req->dest_cid);
			lcapConnection *lcn = findLocalCid(req->hdr.hci.handle,req->dest_cid);
			if (!lcn) return;
			if (lcn->pClient)
				lcn->pClient->lcapEvent(LCAP_EVENT_DISCONNECTED,lcn,0,0);
			
			// the disconnection response uses the same data
			// structure as the disconnection request
			
			lcap_disconnect_request cmd;
			cmd.hdr.hci.handle = req->hdr.hci.handle | 0x2000; 	// word - handle or'd with flags
			cmd.hdr.lcap_d.cid = LCAP_CID;
			cmd.hdr.lcap_c.cmd = LCAP_DISCONNECTION_RESPONSE;
			cmd.hdr.lcap_c.id = req->hdr.lcap_c.id;		// byte - packet number = unique id
			cmd.hdr.lcap_c.cmd_len = 4;
			cmd.dest_cid = req->dest_cid;
			cmd.src_cid  = req->src_cid;
			PRINT_LCAP("<-- LCAP_DISCONNECT_RESPONSE[%d] hci_handle(0x%04x) lcid=0x%04x\n",
				req->hdr.hci.handle,
				cmd.hdr.lcap_c.id,
				lcn->local_cid);
			sendData(&cmd,sizeof(cmd));
			deleteConnection(lcn,true);
			break;
		}
		
		case LCAP_DISCONNECTION_RESPONSE	:
		{
			lcap_disconnect_request	*req = (lcap_disconnect_request *) buffer;	
			PRINT_LCAP("--> LCAP_DISCONNECTION_RESPONSE[%d]  hci_handle(0x%04x) dest_cid=0x%04x src_cid=0x%04x\n",
				req->hdr.lcap_c.id,
				req->hdr.hci.handle,
				req->src_cid,
				req->dest_cid);
			lcapConnection *lcn = findLocalCid(req->hdr.hci.handle,req->src_cid);
			if (!lcn) return;
			if (lcn->pClient)
				lcn->pClient->lcapEvent(LCAP_EVENT_DISCONNECTED,lcn,0,0);
			deleteConnection(lcn,true);
			break;
		}
	
		default:
			PRINT_LCAP("--> UNHANDLED LCAP DATA COMMAND=0x%02x\n",
				   hdr->lcap_c.cmd);
			break;
	}
	
}	// lcapLayer::receiveData()
		




//=====================================
// high level API
//=====================================

void lcapLayer::registerClient(lcapClient *pClient)
	// SDP and RFCOMM at this time
{
	if (m_num_clients >= MAX_LCAP_CLIENTS)
	{
		LOG_ERROR("Could not register client %s - no room!",pClient->serviceName());
		return;
	}
	LOG_LCAP("registering %s service",pClient->serviceName());
	m_pClients[m_num_clients++] = pClient;
}



// void lcapLayer::sendLcapData(lcapConnection *lcn, const u8 *buffer, const u16 length)
// 	// wrap and send the given data packet.
// {
// 	
// }
// 

void lcapLayer::closeConnection(lcapConnection *lcn)
	// you should close the connection and
	// invalidate your pointer when you are
	// done with the connection.
{
	assert(lcn && lcn->device);
	lcap_disconnect_request cmd;
	
	cmd.hdr.hci.handle = lcn->device->handle | 0x2000; 	// word - handle or'd with flags
	cmd.hdr.lcap_d.cid = LCAP_CID;
	cmd.hdr.lcap_c.cmd = LCAP_DISCONNECTION_REQUEST;
	cmd.hdr.lcap_c.id = lcn->device->next_lcap_id++;		// byte - packet number = unique id
	cmd.hdr.lcap_c.cmd_len = 4;
	cmd.dest_cid = lcn->remote_cid;
	cmd.src_cid = lcn->local_cid;

	PRINT_LCAP("<-- LCAP_DISCONNECT_REQUEST[%d] hci_handle(0x%04x) lcid=0x%04x\n",
		lcn->device->handle,
		cmd.hdr.lcap_c.id,
		lcn->local_cid);
	sendData(&cmd,sizeof(cmd));
}



lcapConnection *lcapLayer::startConnection(u8 *addr, u16 psm)
	// Call this method to start an lcap connection
	// to the given machine on the provided PSM.
	// The pointer is valid until a DISCONNECTED
	// or ERROR event.  Note that the psm must
	// map to the psm of a registered service
	// in order for any data to be received.
{
	LOG_LCAP("startLcapConnection(%s,0x%04x)",addrToString(addr),psm);
	hciRemoteDevice *device = m_pHCI->startConnection(addr);
	if (!device) return 0;
	assert(!(device->handle & HCI_HANDLE_ERROR));
	u16 hci_handle = device->handle & 0x0fff;
		
	// create the lcap connection record
	
    lcapConnection *lcn = addConnection(device,psm,0);
	assert(lcn);
	if (!lcn) return 0;
	
	// if hci's got a valid handle
	// start the lcap connection now
	
	if (!hci_handle)
	{
		lcn->lcap_state |= LCAP_STATE_CONNECTING;
	}
	else
	{
		lcap_connect_request cmd;
		
		cmd.hdr.hci.handle = hci_handle | 0x2000; 	// word - handle or'd with flags
		cmd.hdr.lcap_d.cid = LCAP_CID;
		cmd.hdr.lcap_c.cmd = LCAP_CONNECTION_REQUEST;
		cmd.hdr.lcap_c.id = device->next_lcap_id++;		// byte - packet number = unique id
		cmd.hdr.lcap_c.cmd_len = 4;
		cmd.psm = psm;
		cmd.src_cid = lcn->local_cid;

		PRINT_LCAP("<-- LCAP_CONNECTION_REQUEST[%d] hci_handle(0x%04x) lcid=0x%04x\n",
			hci_handle,
			cmd.hdr.lcap_c.id,
			lcn->local_cid);
		sendData(&cmd,sizeof(cmd));
	}
	
	return lcn;

}
	

