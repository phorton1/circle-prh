// there can be multiple adapters on a single machine.
 

#ifndef _bluetooth_h
#define _bluetooth_h

#include "hciLayer.h"
#include "lcapLayer.h"
#include "sdpLayer.h"
#include "rfLayer.h"
#include <circle/types.h>
#include <circle/bt/bttransportlayer.h>


class bluetoothAdapter
{
public:

	bluetoothAdapter();
	
	~bluetoothAdapter(void);

	boolean Initialize(
		btTransportBase *pTransport
		#if HCI_USE_FAT_DATA_FILE
			,FATFS *pFileSystem
		#endif
	);

	hciLayer *getHCILayer() { return &m_HCI; }
	lcapLayer *getLCAPLayer() { return &m_LCAP; }
	sdpLayer *getSDPLayer() { return &m_SDP; }
	rfcommLayer *getRFCommLayer() { return &m_RFCOMM; }


private:

	hciLayer 	m_HCI;
	lcapLayer 	m_LCAP;
	sdpLayer 	m_SDP;
	rfcommLayer m_RFCOMM;
};

#endif
