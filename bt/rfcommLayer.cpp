u8  service_reply =
{
    
    0x00, 0x00    // word - handle or'd with flags
	0x00, 0x00    // hci length (filled in later)
	0x00, 0x00    // lcap length (filled in later)
	0x00, 0x00    // destination channel (remote cid)
	
    SDP_COMMAND_SEARCH_ATTR_RESPONSE,   // command byte == 0x06 = Search for "attributes" of ...
	
    0x00, 0x00,   // txn id = SDP uses word size packet id's
	0x00, 0xf6,   // sdp parameter length
    
    // list of service records
    
    0x35, 0xf4,     // 152+2  + 88+2 = 244 = 0xf4
    
    // sdp service record
    
    0x35, 0x98,                                             // attribute list(152)
          0x09, 0x00, 0x00,                                 //    attr(0x0000,SrvRecHndl)
                0x0a, 0x00, 0x00, 0x00, 0x00,               //         uint32 0x00000000
          0x09, 0x00, 0x01,                                 //    attr(0x0001,SrvClassIDList)
                0x35, 0x03,                                 //        sequence(3)
                      0x19, 0x10, 0x00,                     //             uuid-16 0x1000 (SDPServer)
          0x09, 0x00, 0x04,                                 //    attr(0x0004,ProtocolDescList)
                0x35, 0x0d,                                 //        sequence(13)
                      0x35, 0x06,                           //            sequence(6)
                            0x19, 0x01, 0x00,               //                 uuid-16 0x0100 (L2CAP)
                            0x09, 0x00, 0x01,               //                 uint16 0x0001
                      0x35, 0x03,                           //            sequence(3)
                            0x19, 0x00, 0x01,               //                 split uuid-16 0x0001 (SDP)
          0x09, 0x00, 0x05,                                 //    attr(0x0005,BrwGrpList)
                0x35, 0x03,                                 //        sequence(3)
                      0x19, 0x10, 0x02,                     //             uuid-16 0x1002 (BrowseGroup)
          0x09, 0x00, 0x06,                                 //    attr(0x0006,LangBaseAttrIDList)
                0x35, 0x09,                                 //        sequence(9)
                      0x09, 0x65, 0x6e,                     //             uint16 0x656e
                      0x09, 0x00, 0x6a,                     //             uint16 0x006a
                      0x09, 0x01, 0x00,                     //             uint16 0x0100
          0x09, 0x01, 0x00,                                 //    attr(0x0100,SrvName)
                0x25, 0x12,                                 //         str "Service Discovery"
                      0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x20,
                      0x44, 0x69, 0x73, 0x63, 0x6f, 0x76, 0x65, 0x72,
                      0x79, 0x00,
          0x09, 0x01, 0x01,                                 //    attr(0x0101,SrvDesc)
                0x25, 0x25,                                 //        str "Publishes services to remote devices"
                      0x50, 0x75, 0x62, 0x6c, 0x69, 0x73, 0x68, 0x65,
                      0x73, 0x20, 0x73, 0x65, 0x72, 0x76, 0x69, 0x63,
                      0x65, 0x73, 0x20, 0x74, 0x6f, 0x20, 0x72, 0x65,
                      0x6d, 0x6f, 0x74, 0x65, 0x20, 0x64, 0x65, 0x76,
                      0x69, 0x63, 0x65, 0x73, 0x00,
          0x09, 0x01, 0x02,                                //    attr(0x0102,ProviderName)
                0x25, 0x0a,                                //         str "Microsoft" "prhSystem"
                      'p','r','h','S','y','s','t','e','m', 0x00,
                      //0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66,
                      //0x74, 0x00,
          0x09, 0x02, 0x00,                                //    attr(0x0200,VersionNumList)
                0x35, 0x03,                                //        sequence(3)
                      0x09, 0x01, 0x00,                    //             uint16 0x0100
          0x09, 0x02, 0x01,                                //    attr(0x0201,SrvDBState)
                0x0a, 0x00, 0x00, 0x00, 0x08               //         uint32 0x00000008

    // spp service record

    0x35, 0x58.                                             // attribute list(88)
          0x09, 0x00, 0x00,                                 //      attr(0x0000,SrvRecHndl)
                0x0a, 0x00, 0x01, 0x00, 0x07,               //           uint32 0x00010007
          0x09, 0x00, 0x01,                                 //      attr(0x0001,SrvClassIDList)
                0x35, 0x03,                                 //          sequence(3)
                      0x19, 0x11, 0x01,                     //               uuid-16 0x1101 (SP)
          0x09, 0x00, 0x04,                                 //      attr(0x0004,ProtocolDescList)
                0x35, 0x0c,                                 //          sequence(12)
                      0x35, 0x03,                           //              sequence(3)
                            0x19, 0x01, 0x00,               //                   uuid-16 0x0100 (L2CAP)
                      0x35, 0x05,                           //              sequence(5)
                            0x19, 0x00, 0x03,               //                   split uuid-16 0x0003 (RFCOMM)
                            0x08, 0x03,                     //                   uint8  0x03
          0x09, 0x00, 0x05,                                 //      attr(0x0005,BrwGrpList)
                0x35, 0x03,                                 //          sequence(3)
                      0x19, 0x10, 0x02,                     //               uuid-16 0x1002 (BrowseGroup)
          0x09, 0x00, 0x06,                                 //      attr(0x0006,LangBaseAttrIDList)
                0x35, 0x09,                                 //          sequence(9)
                      0x09, 0x65, 0x6e,                     //               uint16 0x656e
                      0x09, 0x00, 0x6a,                     //               uint16 0x006a
                      0x09, 0x01, 0x00,                     //               uint16 0x0100
          0x09, 0x00, 0x09,                                 //      attr(0x0009,BTProfileDescList)
                0x35, 0x08,                                 //          sequence(8)
                      0x35, 0x06,                           //              sequence(6)
                            0x19, 0x11, 0x01,               //                   uuid-16 0x1101 (SP)
                            0x09, 0x01, 0x02,               //                   uint16 0x0102
          0x09, 0x01, 0x00,                                 //      attr(0x0100,SrvName)
                0x25, 0x05, 0x43, 0x4f, 0x4d, 0x32, 0x37,   //           str "COM27"
          0x09, 0x01, 0x01,                                 //      attr(0x0101,SrvDesc)
                0x25, 0x05, 0x43, 0x4f, 0x4d, 0x32, 0x37,    //           str "COM27"
                
    // continuation byte
    
    0x00
};        














u16 rfcommRequest(u16 hci_handle, u8 *buf, u16 rcid)
    // "Start the RFCOMM multiplexer by sending SABM command on DLCI 0,
    //  and await UA response from peer entity"
    //
    // from "dummies rfcomm" page:  02  2a 20 08 00   04 00 41 00    03 3f 01 1c
    //
    //    02              hci prefix
    //    2a 20 08 00     hci header
    //    04 00 41 00     lcap header
    //    03 3f 01 1c   the hardest 4 bytes ive ever tried to understand
{
    u8 *p = buf;

    // hci and lcap headers

    SET_WORD(p,hci_handle | 0x2000);    // word - handle or'd with flags
    SET_WORD(p,8);                        // hci length 
    SET_WORD(p,4);                        // lcap length
    SET_WORD(p,rcid);                    // destination channel (remote)
    
    // RFCOMM body
    // I'm gonna assume that we actually send msb type bytes ...
    
    SET_BYTE(p,0x03);            // Address
        // bits:  EA=1   C/R   D    Channel(5)
        //
        // EA = low order bit is the "extend address flag"
        //      1 means DONT extend address ...
        // C/R = command(1) or response(0)
        //      1=command (since we are the initiator of the SABM frame)
        // D   = direction bit (part of channel)
        //       initiator sets bit to 1
        //       initiator is defined as "who sent SABM frame to start the multiplexer"
        //       of course, it's NOT set in the dummies example ....
        // Channel = 5 bits for channel 0..31  ... 0 and 31 are reserved ... 0 is DLC0
        
    
    SET_BYTE(p,0x3f);            // Control
        // SABM = b1111p100, where p is the "POLL/FINAL" bit
        // "A command with its P bit set to 1 is used when a response or a series "
        // of responses is wanted from the device at the far end of the link.
    SET_BYTE(p,0x01);            // length (MSB?)
        // also has an EA bit, otherwise we want 0
        // so I think that skips the subsequent length byte:
        // SET_BYTE(p,0);            // length or data
        // and goes directly to the FCS byte
    
    // data - 0 bytes
    
    SET_BYTE(p,0x1c);            // FCS
        // oh, yeah, it's "simple"
        //
        // "To calculate the FCS:
        //      Count up k, the number of bits the FCS will be calculated on. For SABM, DISC,
        //        UA, and DM frames, the frame check sequence is calculated on the address control and
        //        length fields. Then
        //
        //        (a) Calculate the remainder of xk ( x7 + x6 x5 + x4 + x3 + x2 + x1 + 1)
        //            divided modulo 2 by the generator polynomial ( x8 + x2 + x + 1).
        //        (b) Take the contents of the frame that the FCS is calculated over before
        //            any start and stop elements have been inserted, and before any other
        //            extra bits have been inserted. Multiply by x8 and divide by the
        //            generator polynomial ( x8 + x2 + x + 1).(c) Add the results of (a) and (b)
        //            modulo 2, and take the 1’s complement to get the FCS."
        //
        // How these guys start a 5000 page document off with "RFCOMM is a simple protocol",
        // I'll never understand.

    // finished
    
    return PACKET_LEN(buf,p);
}




u16 anySDPRequest(u16 hci_handle, u8 *buf, u16 rcid)
    // try to just find some, any service, or get
    // ANY reply from windows
{
    u8 *p = buf;
    
    u8 LEN = 27;
        // the hci packet length is the full packet len - 4,
        // the lcap packet length is the hci packet length - 4
        // the sdp "parameter length" is the lcap packet length - 5

    // hci and lcap headers

    SET_WORD(p,hci_handle | 0x2000);    // word - handle or'd with flags
    SET_WORD(p,LEN - 4);                // hci length 
    SET_WORD(p,LEN - 4-4);                // lcap length
    SET_WORD(p,rcid);                    // destination channel (remote)

    // SDP header
    
    SET_BYTE(p,SDP_SVC_SEARCH_REQ);            // 0. command byte == Search for services
    SET_WORD(p,be(next_id++));                // 1. id = SDP uses word size packet id's
    SET_WORD(p,be(LEN -4-4-5));                // 2. parameter length
    
    // try a SDP_SVC_SEARCH_REQ for a few different UUIDs
    // it takes two parameters: a list of UUIDs to get and a maximum number to return
    // all SDP packet end in a continuation byte that is zero in our case

    // parameter 1 - a list of UUIDS
    
    SET_BYTE(p,0x35);                         // 0x35 ==> b00111=Data Element Sequence  b001=data size in following 8 bits (byte)
    SET_BYTE(p,0x09);                         // the size is num_uuids * 3 bytes per uuid
    
    // nested data elements
    // these elements use a type-size of 0x19
    // 0x19 ==> b00011=the element is of type UUID,  b001=the element is 16bits (one word) long
    
    SET_BYTE(p,0x19);                         // 
    SET_WORD(p,be(L2CAP_UUID));                // 0x0100
    SET_BYTE(p,0x19);                         // 
    SET_WORD(p,be(RFCOMM_UUID));            // 0x0003
    SET_BYTE(p,0x19);                         // 
    SET_WORD(p,be(SERIAL_PORT_SVCLASS_ID));    // 0x1101

    // parameter 2 - the number of results we'll accept
    
    SET_WORD(p,be(0xFFFF));        // we'll take everything!
    
    // the SDP continuation byte
    
    SET_BYTE(p,0x00);                        // 0 == No continuation (this is a consolidated single packet)
    
    // verify and return the packet length
    
    u8 actual_len = PACKET_LEN(buf,p);
    assert(LEN == actual_len);
    return LEN;

}    // anyFuckingSDPRequest()


