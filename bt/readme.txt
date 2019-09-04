---------------------------
readme.txt for prh bt
---------------------------
    
    After many trials and tribulations, I have decided to make my own Bluetooth
    stack within Circle.
    
    Although I may re-use the publicly available constant #defines [1], the only
    thing I am keeping from the Circle implementation is the notion that there
    exists a transport layer, which is a named device within Circle, and that it
    has RegisterHCIEventHandler (TBTHCIEventHandler *pHandler) and Write(u8 byte)
    methods [2].
    
    Therefore I am including ONE h file from the Circle bt include directory,
    bttransportlayer.h, which contains the definition of TBTHCIEventHandler.
    
    Theoretically this will allow the usage of my bt stack with the existing
    circle usb bluetooth device.  The client merely needs to create, and pass
    in, the transport layer (device) during the system initialization.
    
    I am providing two additional UART transports.  The first, gleaned from
    the Circle code, utilizes the standard PL011 UART and uses code which rst,
    himself, copied from someone else.   The second transport will utilize
    the mini-uart, and uses code gleaned from dwelch.
    
    RANT1 .... unlike the circle code ... in my btack ...

LAYERS
    
    The transport layer does know about the HCI layer. The HCI layer does
    know about the LCAP layer.  In general, lower layers should not know
    about higher layers.  That's the whole concept of layers!
    
    Here are these layers in my btStack:
    
    SPP         - "Serial Port Profile" (protocol), on top of RFCOMM
    RFCOMM      - "Radio Frequency Communication" on top of SDP
    SDP         - "Service Discovery Protocol" on top of L2CAP
    L2CAP       - "Logical Link Control and Adaption" on top of HCI
    HCI         - "Host Controller Interface" on top of the transport layer
    
    and the transport layer, which is anything that lives below HCI:
    
    TRANSPORT   - anything that lives below the HCI layer that can provide
                  bytes to the HCI layer via the RegisterHCIEventHandler
                  callback, and which can receive bytes through a Write()
                  method.


OBJECTS

    uartTransportDevice
    miniUartTransportDevice
    hciLayer
        hciBase
        hciVendor
        hciDiscovery
    lcapLayer
    sdpLayer
    rfcommLayer
    sppLayer
    
    In general, higher layers register with lower layers through
    similar mechanisms.  Each layer has a method to register a
    callback, and by this, each higher layer is called by each
    lower layer to process packets, or parts of them, that are
    specific to the higher layer, as appropriate.
    
    Usually each layer filters out a number of incoming packets
    that are specific to it's protocol.  The lcap layer does not
    send every packet up to the rfcomm layer ... it "handles" many of
    the packets to establish the link-level with remote devices.
    Generally when a layer doesn't know what to do with a packet,
    it passes it up to the higher layer or asserts.
    
    Likewise each layer provides a way of accepting higher level
    data (packets), and wrapping them up in lower level packets
    protocols for transmission, and sending them, ultimately
    to the byte oriented transport layer.
    
    These packets from higher layers on the local (host) device
    are not acted upon by lower layers.  They are just wrapped
    and sent.  So as a higher level layer, you don't send a packet
    to a lower level layer on the same machine to "do" something.
    When a higher layer wants to interact directly with a lower layer
    on the same machine, specific API's must be provided for the higher
    level layer to directly call code in the lower layer.
    
    Since every packet that passes through the system is an HCI packet
    the HCI Layer a relatively complicated beast. Although the packet
    structure is basically the same, this layer has to handle lower
    level ("baseBand" or "device") commands and events like
    OP_CODE_RESET, EVENT_CODE_HARDWARE_ERROR, as well as vendor
    specific command and events, like OP_CODE_DOWNLOAD_MINIDRIVER
    for the rPi's onboard 43438 bluetooth module, as well as the bulk 
    of HCI commands and events for everything from device discovery
    and managment to the negotiation of PIN and encryption key
    exchanges.
    
    From a softare management point of view, therefore, I have
    introduced (at least) three additional objects that are
    invisible to HCI clients, rather than having one source
    file with a single huge switch statement. From an archectural
    point of view, however, these are not additional "layers"
    per se, as they do not generally provide an additional
    level of packet encapsulation for clients, and it is not
    clear that every event is passed through, and discrimated,
    by each of them. They are merely different behaviors of
    the HCI layer.
    
    
HCI PACKETS

    Every packet sent or received through the Bluetooth stack is
    an HCI packet.

    The very first byte in every packet is the HCI packet type prefix:
    
        [ hci_prefix ]

    There are four kinds of packets in the HCI layer.  We are only
    going to handle three of them for now.
    
        0x01 = HCI COMMAND packet
        0x02 = HCI ACL (Ansynronous Data) packet
        0x03 = HCI SCO (Synchronous Data)
        0x04 = HCI EVENT packet
    
    We are not doing SCO synchronous data packets, which, ironically,
    since I'm writing music software, is used for real time audio.
    
    In all cases, the hci_length refers to the number of bytes
    that FOLLOW it.
    

    HCI COMMAND PACKET
    
        HCI COMMAND packets start with a 2 byte OPCODE and a 1 byte length:
        
            [ hci_opcode LSB  ]     with flags
            [ hci_opcode MSB  ]
            [ hci_length      ]

        All HCI Command packets are handled in the HCI layer.
        
    HCI EVENT PACKET
    
        HCI EVENT packets start with a 1 byte EVENT_CODE and a 1 byte length:
        
            [ hci_event_code  ]
            [ hci_length      ]

        All HCI Event packets are handled in the HCI layer.
        
    HCI ACL PACKET
        
        HCI ACL packets start with a 2 byte HCI_HANDLE and a 2 byte length:

            [ fhci_handle LSB  ]     with flags
            [ fhci_handle MSB  ]
            [ fhci_length LSB  ]
            [ fhci_length MSB  ]
        
        All HCI ACL packets are passed up to the LCAP and higher layers.
        
    HCI SCO PACKET
        
        HCI SCO packets start with a 2 byte HCI_HANDLE and a 1 byte length,
        rather than the 2 byte length of an ACL packet.  We are not doing
        SCO packets.

            [ hci_handle LSB  ]     with flags
            [ hci_handle MSB  ]
            [ hci_length byte ]


LCAP PACKETS [3]

    We are only supporting LCAP "connection-oriented channels",
    where the local device communicates with a specific remote device.
    We are not supporting "connectionless" data channels which are
    used for broadcast communications at the LCAP level.
    
    Therefore there is only one basic (outer) packet structure
    in LCAP from our point of view.
    
    An LCAP packet starts with a 2 byte length, and a 2 byte
    channel id:
    
        [ lcap_length LSB ] ... lcap "header"
        [ lcap_length MSB ]
        [ lcap_cid LSB    ]
        [ lcap_cid MSB    ]
    
    LCAP packets come in two different flavors:
    SIGNALLING (command) PACKETS, which have CID==1 
    and which are used to setup other "channels", and 
    DATA PACKETS where are passed on cid's other than 1
    and which are created using signalling (command)
    packet on channel (cid) 1.
    
    The lcap_length refers to the number of bytes that
    follow the CID, and does not include the length of
    the CID (or the length field itself).
    
    Following the lcap_cid (the lcap "header") is the lcap
    "payload" which has two different structures, and so
    essentially defines two different "packet types",
    although both start with the same header, as above.
    
    
    LCAP SIGNALLING (command) PACKET
    
        An LCAP SIGNALLING (command) packet can contain
        one or two COMMANDS.  Our system generally only
        uses one command per packet.
        
        An LCAP COMMAND starts with a 1 byte command
        code, followed by a 1 byte unique "id" determined
        by the initiator of the command, followed by a
        2 byte length, which is then followed by
        command specific data payload.
        
        [ lcap_cmd_code    ]
        [ lcap_txn_id      ]
        [ lcap_cmd_len LSB ]
        [ lcap_cmd_len MSB ]
        ...
        
        The guts of LCAP signalling packets depend on the
        specific command. For example, a CONNECTION_REQUEST,
        to create a new channel takes a known "psm" (constant)
        and a local "source" cid, created by the initiator
        (which for reasons to be explained later, must any unsigned
        word greater than 0x40).
        
        LCAP CONNECTION RQUEST PACKET (example)

            The lcap_cmd_code for the connection request is 0x02.
            
            The psm used for a connection request, in my experience
            is usually 0x0001, which is the bluetooth constant for
            "Service Discovery Protocol" (RFCOMM is 0x0003).
        
            [ lcap_psm LSB 0x01 ]   
            [ lcap_psm MSB 0x00 ]   
            [ local_cid LSB     ]
            [ local_cid MSB     ]
    
            By the way, apart from ERROR and COMMAND COMPLETE HCI events,
            which I did not point out, we have just described the first full,
            completed packet in this document, as the connection request
            packet ends with the local_cid.  So .... here it is:
            
            
            Example Full LCAP Connection Request packet (fixed at 17 bytes in length):
            
                For me, the packet starts with the 0x02, but most discusions
                confusingly leave the prefix byte out and talk about HCI packets
                as if they begin on the hci_handle.
                
                0x02   [ hci_prefix          ] ACL (hci data packet) prefix
                ..     [ hci_handle LSB      ] this is usually referred to as "Start HCI packet"
                ..     [ hci_handle MSB      ] ... you got the hci handle from hci previously
                0x0c   [ hci_length LSB      ] ... 12 bytes of hci payload
                0x00   [ hci_length MSB      ]
                0x08   [ lcap_length LSB     ] start LCAP packet with 12-4=8 bytes of payload
                0x00   [ lcap_length MSB     ]
                0x01   [ lcap_cid LSB        ] ... signal sent on LCAP channel 1
                0x00   [ lcap_cid MSB        ] 
                0x00   [ lcap_cmd_code 0x02  ] start LCAP Signalling (command) packet type
                ..     [ lcap_txn_id         ] ... you create an arbitrary id for tracking response
                0x04   [ lcap_cmd_len LSB    ] start LCAP Command (4 bytes follow length)
                0x00   [ lcap_cmd_len MSB    ]
                0x01   [ lcap_psm LSB 0x01   ] ... a constant in my experience and usage
                0x00   [ lcap_psm MSB 0x00   ]   
                ..     [ local_cid LSB       ] ... you pass in arbitrary handle you create
                ..     [ local_cid MSB       ] ... must be 0x0040 or greater

            and hopefully you will get back a CONNECTION_RESPONSE packet
            that gives you a channel ID on the remote machine which is
            now associated with the local_cid you passed in.
            
            Why am I the first person to write this down in a legible way?
            
    LCAP DATA PACKET
    
        The reason for the LCAP layer is to create logical channels
        for the transmission of data to and from higher level layers
        (between higher level layers on two different machines).
        
        Therefore the data in lcap data packets is unwrapped and
        sent to higher layers, or higher layers give it data, which
        it then (perhaps broken up) and wrapped to then subseuqenty
        be wrapped in HCI packets, with the converse operations
        happening on the other machine.
        
        So LCAP data packets merely consist of the LCAP lcap header
        and data as determined by higher layers.
        

SDP PACKETS
    
    SDP packets use big endian, so the MSB's of, for example, 16 bit
    unsigned numbers, are sent first.  But it's not nearly that simple.
    
    SDP defines it's whole own way of passing data, which is totally
    weird, convoluted, hard to understand, and it is not clear that
    it was worth the effort.  They were trying to save a few bytes
    here and there, and so heavily encoded information into weird
    bit fields.   I will try to explain it, inasmuch as I have to
    encode it. [RANT2]   Starting from Page 2023 of the Bluetooth
    Core Specification in the section titled SERVICE DISCOVERY
    PROTOCOL (SDP) SPECIFICATION:
    
    Here's one of my favorite paragraphs:
    
        "SDP is a simple protocol with minimal requirements on the underlying
        transport. It can function over a reliable packet transport (or even
        unreliable, if the client implements timeouts and repeats requests as
        necessary). SDP uses a request/response model where each transaction consists
        of one request protocol data unit (PDU) and one response PDU. In the case where
        SDP is used with the Bluetooth L2CAP transport protocol, no more than one
        SDP request PDU per connection to a given SDP server shall be outstanding
        at a given instant. In other words, a client shall wait for a response
        to its current request before issuing another request on the same L2CAP
        connection. Limiting SDP to sending one unacknowledged request PDU provides
        a simple form of flow control."
    
    So, they invent a new term, a "PDU", or "Processing Data Unit" to give their
    packet a name, instead of just calling it a packet, and then go on to tell you
    that you can only have one Request PDU (packet) active at a time, and that you
    must wait for it to complete (receive a Response) before sending out another one.
    The DON'T (clearly) tell you what to do if the other guy never answers.
    Are you supposed to shut down the LCAP channel and open a new one to try
    again? Is the other machine's SDP layer transmorgified at his point>?
    Do you have to reboot the other machine?  The needless invention of new
    terminology does not help.
    
    SDP PACKET HEADER
    
        In any case, an SDP packet starts with a one byte "PDU ID" (they could have
        better just called it an SDP Command ID). Most [5] packets are followed by
        a two byte transaction ID, followed by a two byte (Parameter) Length, followed
        by parameter_length bytes of payload. Note that SDP uses big endian when
        passing 16 bit values:
        
            [ pdu_id     ]     .. maybe better as just sdp_cmd.
            [ sdp_id LSB  ]    .. an aribtrary id you create to track responses    
            [ sdp_id MSB  ]
            [ sdp_len LSB ]    .. the "parameter length", the payload length
            [ sdp_len MSB ]    which usually consists of one or more "Data Elements"
    
    SDP CONTINUATION AND ERRORS
    
        One of the data elements is called a "Continuation State
        Parameter" which can indicate that in fact, the data continues
        in a subsequent packet.  [RANT3]
        
        CONTINUATION ELEMENT
        
            [ info_length  ]  maximum 16 .. the length of the SIZE of the cont_len field
            [ cont_len MSB ]  the high order byte of the length of the remaining information
            [ ............ ]  info_length-2 bytes of additional bytes of the LENGTH count
            [ cont_len_LSB ]  if info_length > 1, there will be an LSB
            
        ERROR PDU (packet)
        
            Whoops.  Hold it.  An error response packet is only 3 bytes in length,
            and does not match the PDU header above.  It has PDU_ID == 1.  So
            if PDU_ID = 1, the SDP packet looks like this
            
                [ PDU_ID  0x01   ]
                [ error_code MSB ]
                [ error_code LSB ]
                
            Where the 6 possible error coddes are defined on page 2044 of the
            core bt spec.
            
            So EVERY SDP packet must be decoded as a stream.  There is no
            "SDP header".
            
    SDP DATA ELEMENTS
    
        They describe the Data Elements first in the specification,
        which just makes it harder to understand, but each SDP packet
        header is followed by one or more "Data Elements".
        
        SDP Data Elements must be decoded in a stream oriented fashion.
        You cannot "get to" the 2nd data element in an SDP packet until
        you have figured out the first one.
        
        SDP Data Elements are variable in length, and may be nested.
        In other words, one of the Data Elements is a list of Data Elements.
        This allows SDP to create arbitrarily large and deep data structures.        

        So now I can describe the contents of the more common SDP PDU's
        (packets) which consist of one ore more Data Elements.
        
        A "Data Element" consists of a "Data Header" and the data. The data
        is some number of bytes in length.  The Data Header desribes how
        many bytes that is, and how to interpret those bytes.
        
        DATA ELEMENT HEADER
        
            The Data Header itself is a variable length field consisting
            of a byte which is split into a 5 bit "type" field and a
            3 bit "size index" field, which then may be followed by 0,1,2
            or 4 bytes, which when all taken together, describe the
            type and amount of Data which follows the Data Element
            Header.  [RANT4]
            
                [ type            top 5 bits describe the type
                     size_idx ]   bottom 3 bits is the "size index"
                [ ........... ]   0,1,2 or 4 bytes of additional length info

        
            Here is the text from the spec for the type, and size,
            respectively, along with their footnotes:
            
            DATA ELEMENT HEADER TYPE (bit field)
            
                Type        Valid Size
                Descriptor  Descriptor
                Value       Values              Type Description
                ----------------------------------------------------------------------
                |   0       0                   Nil, the null type
                |   1       0, 1, 2, 3, 4       Unsigned Integer
                |   2       0, 1, 2, 3, 4       Signed twos-complement integer
                |   3       1, 2, 4             UUID, a universally unique identifier
                |   4       5, 6, 7             Text string
                |   5       0                   Boolean*
                |   6       5, 6, 7             Data element sequence,
                |                               a data element whose data field
                |                               is a sequence of data elements
                |   7       5, 6, 7             Data element alternative,
                |                               a data element whose data field
                |                               is a sequence of data elements
                |                               from which one data element
                |                               is to be selected.
                |   8       5, 6, 7             URL, a uniform resource locator
                |   Other                       reserved for future use
                           
                * False is represented by the value 0, and true is represented by the value 1.
                  However, to maximize interoperability, any non-zero value received must be
                  accepted as representing true.

            DATA ELEMENT HEADER SIZE INDEX bit field
            
                Size        Additional
                Index       bits                Data Size
                ------------------------------------------------------------------------
                |   0       0                   1 byte. Exception: if the data element
                |                               type is nil, the data size is 0 bytes.
                |   1       0                   2 bytes
                |   2       0                   4 bytes
                |   3       0                   8 bytes
                |   4       0                   16 bytes
                |   5       8                   The data size is contained in the
                |                               additional 8 bits, which are interpreted
                |                               as an unsigned integer.
                |   6       16                  The data size is contained in the
                |                               additional 16 bits, which are
                |                               interpreted as an unsigned integer.
                |   7       32                  The data size is contained in the
                |                               additional 32 bits, which are
                |                               interpreted as an unsigned integer.


THE UNWRITTEN RULES OF CONNECTING

    At best, the bluetooth specs are confusing with regards to the "protocols" involved
    in establishing "connections" at various layers of the stack.  At worst, they just
    don't provide a clue and I have had to not only reverse engineer other folks code to
    figure out how thigns are supposed to work, but have had to empirically determine
    what needs to be done, in what order, for each layer of the protocol.
    
    HCI
    
        Nowhere is there a simple description of what is needed to connect with
        a remote device, nor a lucid description of what it means to be "paired",
        and what are the two or three ways to acheive that.  Likewise, at this
        point in time, for me, there are no descriptions about what is required
        in the way of settings and negotiation for things like SLOTS, encryption
        bytes, etc.
        
        They just present you with 127 commands and 67 events and never explain
        in what order those command should be issued and those events expectd.
        NCI is the simplest, and seems to work.  It gets worse as you go up the
        stack.
        
        Connecting with HCI
        
            (1) You issue an HCI_OP_LINK_CREATE_CONNECTION command to your controller
                specifying the remote BD_ADDR to connect to.
            (2) You get HCI_EVENT_TYPE_CONNECTION_COMPLETE which indicates success,
                or which may indicate an error.
                
        Being connected to with HCI
        
            (1) You receive an HCI_EVENT_TYPE_CONNECTION_REQUEST from a given BD_ADDR
                with the hci_handle the controller has allocated for the connection.
                There can only be one HCI connection at a time between to machines.
            (2) You respond with the command HCI_OP_LINK_ACCEPT_CONNECTION_REQUEST from
                which point you own the hci_handle lifecycle, or you can issue the command
                HCI_OP_LINK_REJECT_CONNECTION_REQUEST to forego the connection request.
                
        Pairing (as I am doing it)
        
            The difference between a "paired" and "non-paired" set of devices is that
            "paired" devices have exchanged link_keys.  There is apparently a "simple
            pairing mode" that you can put the HCI controller into, but I have not
            messed with (the additional complexity) of doing that.
            
            Pairing is not required to make an HCI connection, nor an L2CAP connection
            for SDP requests.  However, there is an implicit "pairing process" implemented
            on windows and android, whereby if you select the rPi, the UI will pop up
            a window asking for a PIN CODE.  If you enter a pin code, THEN windows
            and android will ask the rPi for a pin code via them issuing the HCI command,
            and you receiving HCI_EVENT_TYPE_PIN_CODE_REQUEST, to which you reply
            with (get this a command that is a reply, sheesh) the hci COMMAND 
            HCI_OP_LINK_PIN_CODE_REQUEST_REPLY, which sends the pin code to the other
            machine (or HCI_OP_LINK_PIN_CODE_REQUEST_NEGATIVE_REPLY if you want to
            blow them off).
            
            Remember that the machines are free to make SDP requests at any time to
            find out what services you provide.  Windows does this during the pairing
            process, for example, to see if there is an SPP service, in which case it
            creates an "outgoing" COMM port for the remote service. So, while the
            pairing process is going on, you are likely to see additional SDP traffic,
            but eventually, if everything worked ...
            
            then you (the rPi) will receive ...
            
            an HCI_EVENT_TYPE_LINK_KEY_NOTIFICATION which contains the link_key for the
            remote machine. Caching this link_key amounts to "pairing" with the device.
            Later, as you climb up the stack, when trying to connect from the rPi to
            the other machine, you will (*may*) receive HCI_EVENT_TYPE_LINK_KEY_REQUEST's
            during, for example, the establishing of an L2CAP connection to RFCOMM,
            to which you merely then reply with HCI_OP_LINK_LINK_KEY_REQUEST_REPLY
            and the link key you have cached.
            
            At this time we do not protect any incoming requests.  You're not supposed
            to for SDP, and if they know the RFCOMM channel of our SPP service we just
            let them connect.  Otherwise, that's it.  The only two LCAP services we provide
            are SDP and RFCOMM, and there is nothing else you can do with an HCI connection,
            nor any reason to try to create an L2CAP connction to any other PSM's.
            
    L2CAP
    
        It starts to get confusing at the L2CAP level.  There are about 20 pages of
        confusing text about the "lockstep" process for establishing an L2CAP connection,
        but it is really not made clear anywhere that you have to
        
        (a) issue an L2CAP connection request.
        (b) you may get a "pending" connection response. or
        (c) you may get a "complete" connection response, BUT
        
        in no way is the L2CAP connection complete and ready to use just because
        you got a "complete" connection response!  Nope!  As I said, the docs
        are rudely unclear, but here is what I have determined is necessary
        for establishing an L2CAP connection to a remote machine.
        
        After you get the connection response, or not ...
        
        (d) wait for a configuration request ...
        (e) issue a configuration response
        (f) issue your own configuration request
        (g) wait for a configuration response
        
        And IF you also had a L2CAP "completed" connection response somewhere
        along the way, when both parties have sniffed each other, THEN AND ONLY
        THEN is the L2CAP connection valid.  If you don't do these steps, then
        you may not receive any response to subsequent efforts to use the L2CAP
        connection, nobody tells you anything!!, and things just won't work.
        It took me almost a month of trying various things to figure this out.
        
    SDP
    
        All I'm gonna say is that although there are no other requirements for
        protocol handshaking with SDP, per se, before you can use the L2CAP
        channel, the whole SDP protocol sucks very badly.  I have never seen
        a more complicated way to represent records always identified by a
        u32, that contain a list of attributes always having u16 ids, who's
        values can be one of a number of atomic types (u8, u16, u32, u64,
        u128, UUID, or string) or a recursive list of such values.
        The way they pack the "type-byte", and then use variable length
        sizes, and the need to know the sizes of things before you unpack
        them ... it sucks.   They could have just used a byte for the type,
        and in all cases, a byte for the number of elements ... or at worst
        a rll encoded length in bytes.
        
        Anyways, the good news is that it is fairly easy to make a connection
        to, or accept a connection from, a remote SDP server.
        
        The other thing to remember is that SDP is big endian, as opposed
        to LCAP which is little endian, so you have to be careful to byte
        swap u16's, u32s, and u64s in the middle of that whole confusing
        process.
        
    RFCOMM
    
        RFCOMM is likewise a horribly documented protocol that is extremely
        difficult to understand, and importantly, has implicit "protocol"
        requirements over and above the description of the packets and
        subpackets defined in the specs.
        
        RFCOMM is worse in some ways than SDP.   That's because not only
        is it big-endian, but it is also conceived of as least-significant-
        bit-first, like you're on the wire receiving one bit at a time, and
        need to make decisions as the bits come in.
        
        So, for example, in RFCOMM almost every byte has an "EA bit" in the least
        significant bit position.  EA means "extended address" and is a way of rll
        encoding multibyte numbers, which is fine.  But RFCOMM's "EA bit" is not only
        in the low order bit, but it is backwards from what you would expect.
        The EA bit is SET if there is NO extended address, and CLEARED if there is
        an extended address (if you should add subseuent bytes to the number you
        are parsing).
        
        Thus, to represent a byte "value" of, say, 0x01, you left shift the value
        by one, and or in the EA bit.  So 0x01 shows up as 0x03 in RFCOMM.  This
        makes it very hard to look at raw packet bytes and understand what is
        happening.
        
        There are also two other low bits (bit 1 and bit 2) for "COMMAND/RESPONSE"
        and "DIRECTION" in a lot of fields (like the "channel number" field) which
        means that you shift right by 3 to get the value.
        
        This whole messy way of represeting things bitwise with flags in the low
        order bits leads to bifurcations in the code.  There are #defines that
        "follow the spec", and assume you have done the right shift and removed
        the EA (and possibly other bits) before comparing them to the constants
        from the spec.
        
        Conversly, there are #defines which assume you HAVE NOT done the shift,
        and which themselves are left shifted versions (masks) of the "spec"
        constants.  These are simpler to apply to the raw packet bytes, but
        it's hard to figure out that they map to specific spec constants.
        
        And we are still only talking about the abstract "contents" of the packets.
        We have not gotten to the arcane nature of the actual constants, and
        the crucial, but nearly impossible to figure out "protocol" of handshaking
        that must occur between two RFCOMM layers before a rfcomm "channel" is
        effectively, and correctly, opened and ready for use.
        
        (a) there is a COMMAND/RESPONSE bit in the address field of every RF comm packet
        (b) there is also a "DIRECTION" bit in the address, which I think is somehow related
            to who is requested to open the rfcomm channel (by sending the original the "SABM"
            "command").  Sometimes the "direction" bit is described as part of the address in
            the spec (i.e. "initiators can only have odd numbered channels from 1..61, 0 and 61
            are reserved") and sometimes not ("the available channels are 0..31")
            
        (c) the FRAME_TYPEs are nowhere really explained and have the weirdest names and
            oddball defined constants.
            
                SABM = a request to open a channel.
                UA   = a response
                UIH  = data, which can be commands to RFCOMM on channel 0,
                       higher level protocol data on other channels
                DM   = disconnection request
                DISC = disconnection response
                
        (d) there is a POLL_FINAL bit which indicates whether this is the "final"
            frame of a type, or if there are more.  It is almost always set,
            except, of course, in the exception that it is not used in UIH packets.
            
            Really, I couldn't make this up if I tried.   
            
                
        (e) every RFCOMM packet ends with a FCS (frame checksum), which is calculated by
            a specific, oft copied, and nearly impossible to understand, formula for
            generating a checksum. Not clear is the fact that the FCS is ONLY USED
            to checksum TWO or THREE bytes in the packet header, though it comes at
            the end of the FULL packet. The full rfcomm packet is NOT checksummed.
            The FCS checksum only applies to the first 3 bytes (address, frame type, and
            data length) of the packet for all frame types except UIH, and only the first
            2 bytes (the address and frame type) of UIH packets.
            
            That's right. All that bs to checksum two or three bytes, and it's not
            even consistent across all packet types ffs.
            
        So, without even talking about the next level of detail (RFCOMM CONTROL COMMANDS),
        there are already 3 direction bits in play:  2 direction indicator bits in the
        first left-shift-or-in-1 address (channel number) byte, and another one in the
        FRAME_TYPE byte. And you have to understand (and copy from someplace) the
        "reversed cyclic redundancy checksum" table and algorithm before you can come
        close to making an RFCOMM connection.   All anybody says is:
        
        then you "send an SABM frame on dlc0"
        
        DLC0 is just another name for "channel 0".
        
        
        ESTABLISHING DLC0
        
            to initate from your machine to remote machine
            
                (A) send SABM frame
                (B) receive UA frame
                
            to accept from the other machine
            
                (A) receive SABM frame
                (B) send UA frame
                
            Of course, you need to have all the C/R, direction, and POLL_FINAL
            bits set correctly, with the correct FCS, or basically nothing happens.
            It was only by using wireshark on the remote device that I could tell
            if my RFCOMM packets were getting through, and if they were effective.
            
            It is not clear when you are sending a command, and when you are
            sending a response.   For example, here is the view from an android
            during the process of a windows laptop initating an rfcomm sssion to it.
            
            884    01:50 (LENOVO-PC2)         (P10)                  RFCOMM  Rcvd SABM Channel=0
                   02 0b 20 08 00 04 00 41 00 [03 3f 01 1c]          -----> addr=03=COMMAND
            885    01:50 (P10)                (LENOVO-PC2)           RFCOMM  Sent UA Channel=0
                   02 0b 20 08 00 04 00 41 00 [03 73 01 d7]          <----- addr=03=COMMAND
            
            RFCOMM is only the bytes in [brackets].
            
                (A) lenovo sends an SABM frame
                
                    addr byte 0x03  = to channel 0 with EA and CR bits set (0 << 3 | 0x03)
                    frame type 0x3f = SABM(0x2f), with the 0x10 POLL_FINAL bit added = 0x3f
                    
                (B) android (P10) responds with UA frame
                
                    addr byte 0x03  = to channel 0 with EA and CR bits set (0 << 3 | 0x03)
                        !!! THE COMMAND BIT IS SET
                    frame type 0x73 = RF_FRAME_UA(0x3f), with the 0x10 POLL_FINAL bit added = 0x73
                        
            OK.  So everything is a command?   What about that pesky "direction" bit?
        
        
        ESTABLISHING DLCN
        
            Now for the next levels of complications.  There are two that play into each other.
            But first, let's get the basic idea that, after getting the UA for channel 0, you are
            supposed to open the actual channel to SPP (the SPP rfcomm channel number that you
            needed SDP to find).  This is done by:
            
                (C) send an SABM frame for channel 'n'
                (D) receive a UA frame for channel 'n'
                
                or, if you are accepting the request to open an RFCOMM channel to your SPP service
                
                (C) receive an SABM frame for channel 'n'
                (D) send a UA frame for channel 'n'
                
            back to our example:
            
                889    01:51 (LENOVO-PC2)         (P10)                RFCOMM  Rcvd SABM Channel=2 (Serial Port)
                       02 0b 20 08 00 04 00 41 00 [13 3f 01 96]         -----> addr=13=COMMAND
                890    01:51 (P10)                (LENOVO-PC2)         RFCOMM  Sent UA Channel=2
                       02 0b 20 08 00 04 00 41 00 [13 73 01 5d]         <----- addr=13=COMMAND
    
            where once again, the lenovo sends a COMMAND and the android responds with a COMMAND ?!?!
            
        DLCN MSC Handshaking
        
            But wait!!   DLCN is not ready to use yet!  If you are reading the specs, your head has
            probably fallen off by now just trying to get this far, but where is the part that explains
            that BEFORE YOU CAN USE ANY NON-ZERO CHANNEL (DLCN) you and the remote machine MUST exchange
            MSC (Modem Status Control) CONTROL packets.
            
            Easier figured out by reverse engineering exsiting code.  Before you can consider the
            SPP channel "open" (and start sending and receiving UIH 'data packets' over it), the
            follwoing packets MUST be exchanged.  In what order?  For sanity, I am only going to
            talk about it from the android P10 "receiving" point of view (cuz that's what I have
            the wireshark file for).
            
            So, the handshaking for channel 'n' takes place via a number of UIH packets sent on
            channel 0, with an embedded 'control command' within the data.  Yup, that's right.
            So 'data' for channel 0, is actually an EMBEDDED sub-packet about it or another channel.
            And just to be as fu as possible, the embedded command has additional EA and CR bits.
            
            
            I have to break one of these down for it to make sense. So in this example, the first handshake
            that occurs is that the android (P10) receives a "Modem Status Control" command from the lenovo:
            
            894    01:51 (LENOVO-PC2)  (P10)   RFCOMM  Rcvd UIH Channel=0 -> 2 MPX_CTRL Modem Status Command (MSC)
                   02 0b 20 0c 00 08 00 41 00 03 ef 09 e3 05 13 8d 70
                   
                   02       = ACL packet prefix
                   0b 20    = HCI handle with flags or'd in
                   0c 00    = HCI length
                   08 00    = LCAP length
                   41 00    = LCAP handle
                   
                   ---- rfcomm packet starts here
                   
                   03       = RFCOMM address (channel 0) with EA and CR bits set
                   ef       = RF_FRAME_UIH frame type imples (for channel 0) RFCOMM "CONTROL COMMAND"
                   09       = command length 4 (4 << 1 | EA=1)
                   
                   ---- nested command
                   
                        e3 = RF_CONTROL_MODEM_STATUS (with EA and COMMAND bit set)
                        
                            ok, now we need to back off and explain the "types" of RFCOMM control commands.
                            They're not defined in the RFCOMM spec itself, which only refers to the TS0.70 spec.
                            They're buried somewhere on the "assigned numbers" page on the bluetooth website.
                            The only document that came close ("RFCOMM FRAME DEFINITION.pdf") described the
                            for the MSC (Modem Status) control command constant as "Type = 0b000111".
                            That look a lot like 0x07 to me. Were that it was that easy.
                            
                            Ah, but (a) there are low order EA and CR bits. Take those out and the above
                            byte becomes 0xe0.  How do you get from b000111 to 0xe0? Well, remember that
                            RFCOMM specs talk about the LEAST SIGNIFICANT BIT FIRST.  So, if you reverse
                            the b000111 pattern and add in the EA=E C/R=C bits (then take them out) you get:
                            
                                    111000EC = 11100000 = 0xE0
                                    
                            simple, eh?
                            
                        
                        05 = (2 << 1 | 1) = nested command length = 2 bytes
                        
                        13 = nested channel number with EA and COMMAND set, and not DIRECTION bit
                             0x10 >> 3 = 0x02 = the rfcomm SPP channel lenovo is trying to open on the android
                        
                        8d = the data for the MSC command is the "signals" byte, which is a bitwise representation
                             of CTR, DTR, etc.  This one is the lenovo telling the android that it is:
                             
                            0x8d =  0b1000 1011         where the low order EA bit must be there, but is ignored
                            
                                #define RF_SIGNAL_FC                        (1 << 1)
                                #define RF_SIGNAL_RTC                       (1 << 2)
                                #define RF_SIGNAL_RTR                       (1 << 3)
                                #define RF_SIGNAL_IC                        (1 << 6)
                                #define RF_SIGNAL_DV                        (1 << 7)
                            
                            FC  = Flow Control bit, set to 1 when a device is unable to accept any RFCOMM
                                frames. When the device is able to receive again, it sends another MSC with the
                                flow control bit set to 0.
                            RTR = ready to receive bytes (??? contradicion, do I have to reverse this?)
                            DV  = this stupid byte is valid
                   
                   ---- finish the full rfcomm packet
                   
                   70   the FCS (frame checksum for ONLY the 2 bytes: RFCOMM address and frame_type)
            
            At this point, note that we have added yet another set of COMMAND/RESPONSE and DIRECTION bits
            and (b) I'm not sure I have the signal bits correct.
            
            The problem is that you must send exactly the correct bits, in the correct packets, in
            the correct order, or nothing happens.  Or worse, it appears to work, but then later
            fails in some weird way.
            
            The bottom line is that (a) you must send AND receive both MSC commands and MSC responses
            before the RF channel (other than zero) is ready to use.  Which means keeping track of them
            and sending them out, as with LCAP, at the right time.  Here's the full MSC exchange for
            the example case:
            

            894    01:51 (LENOVO-PC2)         (P10)                RFCOMM  Rcvd UIH Channel=0 -> 2 MPX_CTRL Modem Status Command (MSC)
                    0000  02 0b 20 0c 00 08 00 41 00 [03 ef 09 [e3 05 13 8d]  ----> addr=03=COMMAND, command=e3=COMMAND
                    0010  70]                                              
            895    01:51 (P10)                (LENOVO-PC2)         RFCOMM  Sent UIH Channel=0 -> 2 MPX_CTRL Modem Status Command (MSC)
                    0000  02 0b 20 0c 00 08 00 41 00 [01 ef 09 [e1 05 13 8d]  <----- addr=01=RESPONSE, command=e1=RESPONSE
                    0010  aa]                                              
            896    01:51 (P10)                (LENOVO-PC2)         RFCOMM  Sent UIH Channel=0 -> 2 MPX_CTRL Modem Status Command (MSC)
                    0000  02 0b 20 0c 00 08 00 41 00 [01 ef 09 [e3 05 13 8d]  <----- addr=01=REPONSE, command=e3=COMMAND 
                    0010  aa]                                              
            
                897    01:51 controller           host                 HCI_EVT Rcvd Number of Completed Packets
                
            898    01:51 (LENOVO-PC2)         (P10)                RFCOMM  Rcvd UIH Channel=0 -> 2 MPX_CTRL Modem Status Command (MSC)
                    0000  02 0b 20 0c 00 08 00 41 00 [03 ef 09 [e3 05 13 8d]  -----> addr=03=COMMAND, command=e3=COMMAND 
                    0010  70]                                              
            899    01:51 (LENOVO-PC2)         (P10)                RFCOMM  Rcvd UIH Channel=0 -> 2 MPX_CTRL Modem Status Command (MSC)
                    0000  02 0b 20 0c 00 08 00 41 00 [03 ef 09 [e1 05 13 8d]  -----> addr=03=COMMAND, command=e1=RESPONSE 
                    0010  70]                                              
            900    01:51 (P10)                (LENOVO-PC2)         RFCOMM  Sent UIH Channel=0 -> 2 MPX_CTRL Modem Status Command (MSC)
                    0000  02 0b 20 0c 00 08 00 41 00 [01 ef 09 [e1 05 13 8d]  <----- addr=01=RESPONSE, command=e1=RESPONSE
                    0010  aa]
                                
            
            Notice:
            
                (a) everybody talks about embedded channel 0x13 = 0x02<<3  + EA + COMMAND bit
                    ... in every case
                (b) whenever the android receives something, the outer level C/R bit for DLC0
                    is set, and whenever it sends something, it is cleared.
                (c) that the lenovo sends the exact same command twice in packets 894 and 898, thus
                (d) the p10 sends the exact same response twice in packets 895 and 900
                
                Thus the android (receptor of the initial SABM frame) only sends outer level responses,
                even when it sending an inner level command, and it only receive outer level commands,
                even when it is receving an inner level response.
                
        
        PARAMETER NEGOTIATION
        
            I notice that the lenovo are exchanging one pair of "PN" (Parameter Negotiation)
            control commands, both for DLC0 and DLCN.
            
            I think this is the missing piece.
            
        
STATUS

    The rPi can do Inquiries
    The rPi can initiate HCI and LCAP connections and do SDP requests to any machine
    The rPi can accept HCI and LCAP connection and respond to SDP requests with a contant set of service records
    The rPi can "pair" with a remote machine and cache, and use, the link_key provided by that machine in subsequent requests
    
    RFCOMM PROS:
    
        the rPi can initiate an RFCOMM connection to either machine
            - to lenovo channel 3 with SPP (an "incoming" COMM port) open
            - to android channel 2 with SPP (a "bluetooth" terminal program running
        the rPi can accept an RFCOMM connection from the P10 (bluetooth serial program)
        I can send and receive bytes to and from the P10
        I can send bytes to the lenovo
        
    RFCOMM CONS:
    
        any connections I establish (using just MSC handshaking) are closed by the
            remote machine after a given timeout (2 mins for lenovo, 20 secs for P10)
            
        the rPi CANNOT accept an RFCOMM connection from the lenovo (from a
            particular "outgoing" COMM port). It hangs, I think having to do with
            "RFCOMM Parameter Negotiation" handshaking
        
        any attempts to SEND bytes from the lenovo to the rPi HARD CRASH windows
            with a core-dump and requiring a hard reset of the machine ..
    
    
WHERE I'M AT

    So, I am reviewing in detail the RFCOMM handshaking that takes place,
    including perhaps encoding the need to do PN handshakking in both DLC0
    as well as DLCN.
    
    My gut feeling is that a problem with my RFCOMM handshaking (and not
    something in the HCI or LCAP layers) is leading to the disconnects.
    
    I don't know what is causing the crash on windows.  I thought it was
    the lack of SDP.  Maybe it still is the lack of SPP and some kind
    of baud_rate or other negotiation that needs to take place.
    
    ----------------------------    
    
    When I can get reliable connections in both directions, along with
    communication in both directions, working on both windows and android,
    I will consider my little bluetooth sub-project completed, clean up,
    and factor the code somewhere, and move back to my real, music hardware
    project.




--------------------------------------------------

notes:

*         It is worth noting that "LCAP Commands" (signals) 
        are byte sized and less than 0x40, and that is why
        (it took a while to figure out) LCAP "channel ids"
        start at 0x40, even though they two bytes in length.
        Because it is little endian, 
        



0 - Note on endian-ness.  Layers below SDP uniformly use little endian,
    where the LSB is transmitted first, and generally only use bytes and
    words. SDP uses big endian and a bewildering array of data types.

1 - There are a glom of defines for bt layers, available all over
    the place.  I am sick of 100 line comments at the top of source
    files for licenses with no comments in the code. I am going
    to make this project look like I want.  Licenses may be found
    in licenses.txt, and apply to the source code generally.
    
2 - My stack also the publicly available hcd files and utliizes
    the circle convertool.exe program to convert them to H files.
 
3 - I hate typing "L2CAP", so henceforth it is just LCAP

4 - I think circle confusingly put "Device Discovery (Inquiry)"
    into the "btLogicalLayer".  Perhaps the user of "logical"
    was too close to the "L" in L2CAP, but from a layer point
    of view, I expected it to be the LCAP layer.  I'm not sure
    what's "logical" about the circle implementation except that
    it is half conceived and does not even make a provision for
    the introduction of a higher level LCAP layer without a
    nearly total overhaul.

5 - "Most SDP" packets, except for SDP ERROR PDU's (packets).
    See what I mean.  I had to glean this from the spec.
    They start off describing the generic header, then later
    contradict themselves, describing a packet that does not
    match the header. 


RANT1
    
    It has been amazingly complicated to get this far.  For all of the
    many thousands of pages of Bluetooth specifications, dozens of stack
    implementations, thousands of development projects utilizing those
    stacks, providing software to billions of people, who commonly have
    two, three, or more bluetooth devices in their homes, it was
    ridiculously complicated to put together a rudimentary understanding
    of the byte structure of the packets passed between layers.
    The ONLY example where someone showed the actual bytes, and how
    to interpret them was, ironically, from Dmitry Pakhomenko, at
    circuits@home, and related to the arduino USB Host Shield Library,
    which I've already spent months trying to work with.
    
    So it would be a passing goal of mine to also explain, in simple
    terms, without endlessly nested acronyms, wtf is going on.
    It's really not that complicated, but it pisses me off to
    be in the middle of 3000 page document, trying to understand
    3/5 bit packing in a nested specification, that refers to to
    a table 1500 pages later in the manual, that then refers to
    another specification ... nobody fucking shows you the bytes!!
    
    Nowhere, on the billions of webpages, after hundreds and hundreds
    of searches, did anyone simply show a RFCOMM packet, nested inside
    of an LCAP packet, nested inside of an HCI packet, WITH THE BYTES.
    
    Sheesh.
    
    For this project, that made things more complicated because I am
    not certain that the most basic Circle implementation makes sense
    architecturally.  For some unexplainable reason, all of his layers
    have forward knowledge of the layers above them.  The transport
    layer was deciding what kind of HCI packet was being received,
    and was not built to handle the most basic ACL (data packets).
    
    Some munging is necessary with the rPi and the poorly documented
    43438 Bluetooth module as has to be bootstrapped with firmware
    (the hcd files) utilizing Vendor Specific HCI protocols.  So,
    essentially, the HCI layer MUST know if it is running on a rPi
    UART, so that it can initalize the onboart BT module.
    
    But ...

RANT2

    It pisses me off when theres a 3000 page document, that uses
    convoluted bit packing methods, and the authors use terms
    like "is a simple" or "can be easily".  Like in the SDP spec
    where it says you can "easily" convert a 16 bit bluetooth
    reserved UUID into a full UUID by merely multiplying it by
    2^96 and then subtracing a constant (the bluetooth base UUID)
    that you ahve to find on another page, 1500 pages away,
    or worse yet, in a registry on the website.
    
    The guys that wrote this stuff, and the guys that wrote
    the stacks, and in general the whole bluetooth community
    is full of shit.  There is this sort of expectation that
    you come to the party armed with a previous understanding
    of the hundreds of acronyms, the complicated inter
    relationships between protocols, layers, models, etc,
    and live your life as a full time IEEE specification writer.
    
    I think the net result, perpetuated by those that be, was
    to create a mystique and ongoing revenue stream for "experts"
    and authors of bluetooth "how to" books.
    
    Like I said, I spent a month googling this stuff, and it
    was nearly impossible, I had to look folks source code and
    figure out what it was doing, to come up with a basic picture
    of a simple LCAP Connection Request Packet.  Try to find
    an example of a complete SDP Service Discovery packet
    (the bytes) on the net.  It will nearly kill you.
    
    When you get to the point where you are reverse engineering
    linux device drivers in order to understand how to write
    a piece of code (see "dwc host controller") you know that
    somewhere, sometime, somebody upstream from you made your
    life difficult for no good reason except to secure a
    financial reward for keeping you from understading something.


RANT3

    Try these two paragraphs on for size:
    
        "Some SDP requests may require responses that are larger than can
        fit in a single response PDU. In this case, the SDP server shall
        generate a partial response along with a continuation state parameter.
        The continuation state parameter can be supplied by the client in a
        subsequent request to retrieve the next portion of the complete response.
        The continuation state parameter is a variable length field whose first
        byte contains the number of additional bytes of continuation information
        in the field.
        
        The format of the continuation information is not standardized among SDP
        servers. Each continuation state parameter is meaningful only to the SDP
        server that generated it. The SDP server should not expose any sensitive
        information, such as internal data structures, in the continuation state
        parameter. The SDP server should validate a continuation state parameter
        supplied by the client before using it."
    
    Nice!  So I *may* get a "variable lengthed" field that tells me, while I am
    in the middle of building a complicated data structure from an SDP packet,
    that I must now issue another request, passing the "continuation state
    parameter from the first packet", save the state of my structure building,
    wait for a subsequent response, and continue building the data structure
    again from the point I left off. Oh and they never did tell me what to do
    if the other guy doesn't answer.  Did I mention it was complicated?


RANT4

    Pages 2038 and 2039 of the BT Core spec are perhaps the worst examples
    I have ever seen from both a design, and a documentation, perspective.
    It took the best part of a day to understand these two pages.
    There are no good examples, and it prodigously refers to terms,
    types, and values that are not found in the section, spec, or
    easily online.  You can pass a list of UUIDs. What is a u16 UUID
    passed as?   What is the value of the SDP discovery protocol u16
    UUID.  Why didn't ANYONE ever document the bytes you need to send,
    and should expect to receive, for a simple SDP discovery for
    RFCOMM?
    
    
    
    
    
