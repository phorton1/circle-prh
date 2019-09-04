#ifndef _RFCOMM_H_
#define _RFCOMM_H_

#define FRAME_POLL_FINAL                        (1 << 4)

#define RFCOMM_CONTROL_DLCI                     0
#define RFCOMM_MAX_OPEN_CHANNELS                5

// Enum for the types of RFCOMM frames which can be exchanged on a Bluetooth channel

enum RFCOMM_Frame_Types_t
{
	RFCOMM_Frame_DM    = 0x0F, // Disconnected Mode Field 
	RFCOMM_Frame_DISC  = 0x43, // Disconnect Field 
	RFCOMM_Frame_SABM  = 0x2F, // Set Asynchronous Balance Mode Field 
	RFCOMM_Frame_UA    = 0x63, // Unnumbered Acknowledgement Field 
	RFCOMM_Frame_UIH   = 0xEF, // Unnumbered Information with Header check Field 
};

enum RFCOMM_Channel_States_t
{
	RFCOMM_Channel_Closed      = 0,
	RFCOMM_Channel_Configure   = 1,
	RFCOMM_Channel_Open        = 2,
};


typedef struct
{
	uint8_t  DLCI;
	uint8_t  State;
	uint8_t  Priority;
	uint16_t MTU;
	uint8_t  ConfigFlags;
	struct
	{
		uint8_t Signals;
		uint8_t BreakSignal;
	} Remote;
	struct
	{
		uint8_t Signals;
		uint8_t BreakSignal;
	} Local;
} RFCOMM_Channel_t;

		
	
#endif