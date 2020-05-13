// midiEvent.h
//
// Defines a context free midi event and dispatching system
// that makes use of global variables to keep a linked list
// of midiEventHandlers.
//
// Each midiEventHandler is associated with a callback method
// and an object that will be called if the particular midi
// event matches the filter criteria.
//
// The filter criteria are the cable, channel, msg, param1, and param2,
// where -1 means "take any".


#ifndef _midiEvent_h
#define _midiEvent_h

#include <circle/types.h>

//--------------------------------------------------------
// my higher level abstraction of midi event types,
//--------------------------------------------------------
// as the case may be

typedef enum
{
	MIDI_EVENT_TYPE_NOTE, 		// returns MIDI_EVENT_NOTE_ON and MIDI_EVENT_NOTE_OFF 
	MIDI_EVENT_TYPE_CC,    		// returns MIDI_EVENT_CC 
	MIDI_EVENT_TYPE_INC_DEC,   	// returns MIDI_EVENT_INCREMENT or MIDI_EVENT_DECREMENT
}	midiEventType_t;


// NRPN event registration (B0) with various others
// 	  The MPD218 can return continuous values for the rotary controllers
//    via increment (0x60) and decrement (0x61) messages.
//    These appear to be preceded by 0x63 with the MSB and
//    0x62 with the LSB, and followed by 0x63 and 0x62 with 0x7F.
//
//             b0  63  msb
//             b0  63  lsb
//             b0  60  increment_value
//
//    The loop will continually monitor incoming msb/lsb pairs,
//    and on the b0,61,value,  will return it to the client because
//    it matches the current nrpn registers.


#define MIDI_EVENT_NOTE_ON   0x09		// after being rightshifted from leading status byte
#define MIDI_EVENT_NOTE_OFF  0x08
#define MIDI_EVENT_CC        0x0B
#define MIDI_EVENT_MSB       0x63
#define MIDI_EVENT_LSB       0x62
#define MIDI_CC_DECREMENT 	 0x61
#define MIDI_CC_INCREMENT 	 0x60



class midiEvent;		// forward;


typedef void (*handleMidiEventFxn)(void *, midiEvent *event);


class midiEvent
{
	public:

		~midiEvent() {}
	
		midiEvent(
			s16 cable,
			s16 channel,
			s16 msg,
			s16 value1,
			s16 value2) :
				m_cable(cable),
				m_channel(channel),
				m_msg(msg),
				m_value1(value1),
				m_value2(value2)
		{}

		
		s16 getCable() const		{ return m_cable; }
		s16 getChannel() const  	{ return m_channel; }
		s16 getMsg() const   		{ return m_msg; }
		s16 getValue1() const  		{ return m_value1; }
		s16 getValue2() const  		{ return m_value2; }
		
	protected:
		
		s16 m_cable;
		s16 m_channel;
		s16 m_msg;
		s16 m_value1;
		s16 m_value2;
		
};	// midiEvent


class midiEventHandler : public midiEvent
{
	public:
		
		~midiEventHandler() {}
	
		static midiEventHandler *getFirstHandler()			{ return m_sFirstHandler; }
		midiEventHandler *getNextHandlerHandler()	{ return m_pNextHandler; }
		static void dispatchMidiEvent(midiEvent *pEvent,u8 *nrpn);
		
		static void registerMidiHandler(
			void *pObject,
			handleMidiEventFxn pMethod,
			s16 cable,
			s16 channel,
			midiEventType_t type,
			s16 value1,
			s16 value2);
		static void unRegisterMidiHandler(
			void *pObject,
			handleMidiEventFxn pMethod,
			s16 cable,
			s16 channel,
			midiEventType_t type,
			s16 value1,
			s16 value2);
	
	protected:

		midiEventHandler(
			void *pObject,
			handleMidiEventFxn pMethod,
			s16 cable,
			s16 channel,
			midiEventType_t type,
			s16 value1,
			s16 value2 );
			
		void *m_pObject;
		handleMidiEventFxn m_pMethod;

		midiEventType_t m_type;

		midiEventHandler *m_pNextHandler;
		midiEventHandler *m_pPrevHandler;	
		
		static midiEventHandler *m_sFirstHandler;
		static midiEventHandler *m_sLastHandler;
		
};	// midiEventHandler


#endif  // !_midiEvent_h
