
#ifndef AudioStream_h
#define AudioStream_h

#include "AudioDevice.h"
#include "AudioSystem.h"


class AudioStream :  public AudioDevice
{
public:

	~AudioStream();

	AudioStream(
        u16 num_inputs = 0,
        u16 num_outputs = 0,
        audio_block_t **input_queue = 0,
        u16 num_controls = 0,
        audioControl_t *control_setup = 0);

	u16 getNumInputs()		    	{ return m_numInputs; }
	u16 getNumOutputs()				{ return m_numOutputs; }
    
    AudioStream *getNextStream()    { return m_pNextStream; }
    
	u16		getUpdateDepth()			{ return m_updateDepth; }
	u32 	getCPUCycles()  	        { return m_cpuCycles; }
	u32 	getCPUCyclesMax()	        { return m_cpuCyclesMax; }
	void 	resetStats()                { m_cpuCycles=0; m_cpuCyclesMax=0; }
    
    AudioStream *getConnectedInput(u8 channel, u8 *src_channel);
    AudioStream *getFirstConnectedOutput(u8 channel, u8 *dest_channel);

protected:
friend class AudioSystem;
friend class AudioConnection;
    
	virtual void update(void) {}
	void transmit(audio_block_t *block, unsigned char index = 0);
	audio_block_t *receiveReadOnly(unsigned int index = 0);
	audio_block_t *receiveWritable(unsigned int index = 0);

	void	setUpdateDepth(u16 depth)	{ m_updateDepth = depth; }

    // member variables
    
	u16 			m_numInputs;
	u16     		m_numOutputs;
	audio_block_t **m_inputQueue;
    AudioStream    *m_pNextStream;
    u16             m_numConnections;
	AudioConnection *m_pFirstConnection;
	u16             m_updateDepth;
	u32      		m_cpuCycles;
	u32      		m_cpuCyclesMax;
    
    static u16      s_numStreams;

};


#endif	// !AudioStream_h
