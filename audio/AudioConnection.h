 
#ifndef AudioConnection_h
#define AudioConnection_h

#include "AudioTypes.h"

class AudioConnection
{
public:
	
	AudioConnection(
			AudioStream &source,
			AudioStream &destination) :
		m_src(source),
		m_dest(destination),
		m_srcIndex(0),
		m_destIndex(0),
		m_pNextConnection(0)
	{
		m_bConnected = false;
		connect();
	}

	AudioConnection(
			AudioStream &source,
			unsigned char sourceOutput,
			AudioStream &destination,
			unsigned char destinationInput) :
		m_src(source),
		m_dest(destination),
		m_srcIndex(sourceOutput),
		m_destIndex(destinationInput),
		m_pNextConnection(0)
	{
		m_bConnected = false;
		connect();
	}

	~AudioConnection()
	{
		disconnect();
	}
	
	void disconnect(void);
	void connect(void);
	
protected:
	friend class AudioSystem;
	friend class AudioStream;
	
	AudioStream &m_src;
	AudioStream &m_dest;
	unsigned char m_srcIndex;
	unsigned char m_destIndex;
	AudioConnection *m_pNextConnection;
	bool m_bConnected;
	
};


#endif	// !AudioConnection_h
