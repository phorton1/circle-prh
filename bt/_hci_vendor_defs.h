// Vendor specific commands and packet structures

#ifndef __hci_vendor_defs_h
#define __hci_vendor_defs_h

#include "_hci_defs.h"

#define HCI_OP_VENDOR_DOWNLOAD_MINIDRIVER	(HCI_OP_TYPE_VENDOR | 0x02E)
#define HCI_OP_VENDOR_WRITE_RAM				(HCI_OP_TYPE_VENDOR | 0x04C)
#define HCI_OP_VENDOR_LAUNCH_RAM			(HCI_OP_TYPE_VENDOR | 0x04E)


struct hci_vendor_command
{
	hci_command_header hdr;
	u8	data[255];
} 	PACKED;


#endif
