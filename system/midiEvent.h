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


#define MIDI_EVENT_NOTE_ON   0x09		// after being rightshifted from leading status byte
#define MIDI_EVENT_NOTE_OFF  0x08
#define MIDI_EVENT_CC        0x0B



class midiEvent;		// forward;


typedef void (*handleMidiEventFxn)(void *, midiEvent *event);


class midiEvent
{
	public:

		~midiEvent() {}
	
		midiEvent(
			s8 length,
			s8 cable,
			s8 channel,
			s8 msg,
			s8 value1,
			s8 value2 ) :
				m_length(length),
				m_cable(cable),
				m_channel(channel),
				m_msg(msg),
				m_value1(value1),
				m_value2(value2)
		{}

		
		s8 getLength()			{ return m_length; }
		s8 getCable()			{ return m_cable; }
		s8 getChannel() const   { return m_channel; }
		s8 getMsg() const   	{ return m_msg; }
		s8 getValue1() const   	{ return m_value1; }
		s8 getValue2() const   	{ return m_value2; }
		
	protected:
		
		s8 m_length;
		s8 m_cable;
		s8 m_channel;
		s8 m_msg;
		s8 m_value1;
		s8 m_value2;
		
};	// midiEvent


class midiEventHandler : public midiEvent
{
	public:
		
		~midiEventHandler() {}
	
		static midiEventHandler *getFirstHandler()			{ return m_sFirstHandler; }
		midiEventHandler *getNextHandlerHandler()	{ return m_pNextHandler; }
		static void dispatchMidiEvent(midiEvent *pEvent);
		
		static void registerMidiHandler(
			void *pObject,
			handleMidiEventFxn pMethod,
			s8 cable,
			s8 channel,
			s8 msg,
			s8 value1,
			s8 value2);
		static void unRegisterMidiHandler(
			void *pObject,
			handleMidiEventFxn pMethod,
			s8 cable,
			s8 channel,
			s8 msg,
			s8 value1,
			s8 value2);
	
	protected:

		midiEventHandler(
			void *pObject,
			handleMidiEventFxn pMethod,
			s8 cable,
			s8 channel,
			s8 msg,
			s8 value1,
			s8 value2 );
			
		void *m_pObject;
		handleMidiEventFxn m_pMethod;
		midiEventHandler *m_pNextHandler;
		midiEventHandler *m_pPrevHandler;	
		
		static midiEventHandler *m_sFirstHandler;
		static midiEventHandler *m_sLastHandler;
		
};	// midiEventHandler


#endif  // !_midiEvent_h
