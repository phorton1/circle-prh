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
// starting with MPD218, as the case may be

typedef enum
{
	MIDI_EVENT_TYPE_NOTE, 		// returns MIDI_EVENT_NOTE_ON and MIDI_EVENT_NOTE_OFF 
	MIDI_EVENT_TYPE_CC,    		// returns MIDI_EVENT_CC 
	MIDI_EVENT_TYPE_INC_DEC1,   // returns MIDI_EVENT_INC/DEREMENT with programmed value
	MIDI_EVENT_TYPE_INC_DEC2,	// returns MIDI_EVENT_INC/DECREMENT with 1+ or 7f- for velocity
	
}	midiEventType_t;

// The main problem with CC's and knobs on the MPD218 is that values "jump"
// on the first touch.  That's necessary to use a footpedal (at this time).

// The MPD218 can return continuous values for the rotary controllers
// in two different ways.  The first way DEC1, is complicated
// and involves NRPNs, and two distinct messages. DEC2 is
// somewhat simpler and returns the CC with velocity numbers.
//
// DEC1 receives increment (0x60) or decrement (0x61) messages,
//    along with four NRPN MSB (0x63) and and LSB (0x62) messages.
//    The inc/dec are preceded by the programmed msb/lsb's, and
//    followed by 7f's to presumably clear the nrpn msb/lsb.
//
//             b0  63  msb
//             b0  62  lsb
//             b0  60  increment_value
//             b0  63  7f
//             b0  62  7f
//
//    The kernel continually monitors incoming msb/lsb pairs, on the
//    62,63 messages and send the NRPN values to dispatch() for it to use.
//    The client registers on MIDI_EVENT_TYPE_INC_DEC1 and the msb/lsb
//    pair, and will receive MIDI_CC_INCREMENT/DECREMENT messages with
//    the value programmed into the MPD.
//
// DEC2 receives CC messages values of positive values of 1,2... for increment
//   or 7f,7e,... for decrement, based on the velocity, where presumably the
//   0x40 bit is used for the sign. It uses special handling in dispatch
//   to build inc/dec events, and probably works the best for my uses.


#define MIDI_EVENT_NOTE_ON   0x09		// after being rightshifted from leading status byte
#define MIDI_EVENT_NOTE_OFF  0x08
#define MIDI_EVENT_CC        0x0B
#define MIDI_EVENT_MSB       0x63
#define MIDI_EVENT_LSB       0x62
#define MIDI_CC_DECREMENT 	 0x61
#define MIDI_CC_INCREMENT 	 0x60



class midiEvent;		// forward;
class midiEventHandler;


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
		friend class midiEventHandler;
		
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
