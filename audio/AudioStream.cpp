#include "AudioStream.h"
#include "AudioConnection.h"


AudioStream::~AudioStream() {}


AudioStream::AudioStream(
        u16 		    num_inputs,         
        u16 		    num_outputs,        
        audio_block_t **input_queue,
        u16             num_controls,
        audioControl_t *control_setup) :
    AudioDevice(num_controls,control_setup),
	m_numInputs(num_inputs),
	m_numOutputs(num_outputs),
	m_inputQueue(input_queue)

{
	m_instance          = 0;
    m_numConnections    = 0;
	m_pFirstConnection  = 0;
	m_updateDepth       = 0;
    
    // initialize the input queue for the client
    // as the typical usage is to pass it to us
    // uninitialized in the ctor ...
    
    if (m_inputQueue)
        *m_inputQueue = 0;
        
    m_pNextStream       = 0;
    if (AudioSystem::s_pLastStream)
        AudioSystem::s_pLastStream->m_pNextStream = this;
    else
        AudioSystem::s_pFirstStream = this;
    AudioSystem::s_pLastStream = this;
    AudioSystem::s_numStreams++;
    
    resetStats();
}
    




void AudioStream::transmit(audio_block_t *block, unsigned char index)
{
	for (AudioConnection *c = m_pFirstConnection; c != NULL; c = c->m_pNextConnection)
	{
		if (c->m_srcIndex == index)
		{
			if (c->m_dest.m_inputQueue[c->m_destIndex] == NULL)
			{
				c->m_dest.m_inputQueue[c->m_destIndex] = block;
				block->ref_count++;
			}
		}
	}
}


audio_block_t *AudioStream::receiveReadOnly(unsigned int index)
{
	if (index >= m_numInputs)
		return NULL;
	audio_block_t *in = m_inputQueue[index];
	m_inputQueue[index] = NULL;
	return in;    
}


audio_block_t *AudioStream::receiveWritable(unsigned int index)
{
	if (index >= m_numInputs)
		return NULL;
	audio_block_t *in = m_inputQueue[index];
	m_inputQueue[index] = NULL;
	if (in && in->ref_count > 1)
	{
		audio_block_t *p = AudioSystem::allocate();
		if (p) memcpy(p->data, in->data, sizeof(p->data));
		in->ref_count--;
		in = p;
	}
	return in;    
}


AudioStream *AudioStream::getConnectedInput(u8 channel, u8 *src_channel)
    // return the AudioStream, if any, that is connected to this
    // streams nth input
{
    for (AudioStream *p=AudioSystem::getFirstStream(); p; p=p->getNextStream())
    {
        for (AudioConnection *con=p->m_pFirstConnection; con; con=con->m_pNextConnection)
        {
            if (&con->m_dest == this && con->m_destIndex == channel)
            {
                *src_channel = con->m_srcIndex;
                return &con->m_src;
            }
        }
    }
    return NULL;
}


AudioStream *AudioStream::getFirstConnectedOutput(u8 channel, u8 *dest_channel)
{
    if (m_pFirstConnection)
    {
        *dest_channel = m_pFirstConnection->m_destIndex;
        return &m_pFirstConnection->m_dest;
    }
    return NULL;
}


