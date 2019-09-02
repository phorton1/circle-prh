//
// Circle Squared - for lack of a better name
// Copyright (c) 2019 Patrick Horton
//
// A generalized framework for creating audio-midi-ui applications
// in bare metal on the Raspberry Pi initially based ond the Circle
// rPi Bare Metal OS and the Teensy Audio Library.
//
// The Circle rPi Bare Metal OS is
// Copyright (x) 2017-2010  R. Stange, rsta2@o2online.de
//
// The Teensy Audio Library is
// Copyright (c) 2010, Paul Stoffregen, paul@pjrc.com

#ifndef _midiEvent_h
#define _midiEvent_h

#include <circle/types.h>

#define MIDI_MSG_CC   0xb9
#define MIDI_CC_VOL   0x07
	// wrong but I need something

class midiEvent
{
	public:
	
		midiEvent(
			s8 channel,
			s8 msg,
			s8 value1,
			s8 value2 ) :
				m_channel(channel),
				m_msg(msg),
				m_value1(value1),
				m_value2(value2)
		{}

		~midiEvent() {}
		
		s8 getChannel() const   { return m_channel; }
		s8 getMsg() const   	{ return m_msg; }
		s8 getValue1() const   	{ return m_value1; }
		s8 getValue2() const   	{ return m_value2; }
		
	protected:
		
		s8 m_channel;
		s8 m_msg;
		s8 m_value1;
		s8 m_value2;
		
		midiEvent *m_pNext;
		midiEvent *m_pPrev;
		
};	// systemEvent




//------------------------------------
// midiEventHandler
//------------------------------------


class midiEventHandler 
{
	public:
		
		virtual u8 handleMidiEvent(midiEvent *event)
			// return nonzero to stop propogation?
			{ return 0; }
};


class midiDevice : public midiEventHandler
{
public:	
	
	~midiDevice() {}
	
	midiDevice() :
		m_instance(0) 	{}
		
	virtual const char *getName() = 0;
    virtual u16 getInstance() { return m_instance; }

protected:
	
    u16 m_instance;
	
};



#endif  // !_systemEvent_h
