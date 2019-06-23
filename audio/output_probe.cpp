#include "output_probe.h"
#include <circle/sched/scheduler.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/alloc.h>


#define log_name "aprobe"

#define AUDIO_PROBE_ONE_SHOT   1
    // If set, calling start() will sample one complete buffer (1 second)
    // and then stop (setting m_running appropriately).  If not set,
    // the buffer will continually be circularly filled until stop()
    // is called.

#define PROBE_BUFFER_BYTES  (PROBE_WIDTH * 2)



//------------------------------------------
// AudioProbe initialization, etc
//------------------------------------------

AudioProbe *AudioProbe::s_pAudioProbe = 0;
int16_t AudioProbe::m_buffer[PROBE_CHANNELS][PROBE_SAMPLES];


AudioProbe::AudioProbe(u16 skip) :
    AudioStream(PROBE_CHANNELS, inputQueueArray),
    m_skip(skip)
{
    m_running = 0;
    initBuffers();
    s_pAudioProbe = this;
}    


void AudioProbe::begin()
{
}


void AudioProbe::initBuffers()
{
    m_offset = 0;
    for (u16 i=0; i<PROBE_CHANNELS; i++)
    {
        memset(m_buffer[i],0,PROBE_SAMPLES*sizeof(s16));
    }
}

    
void AudioProbe::start()
{
    LOG("start()",0);
    initBuffers();
    m_running = true;
}

void AudioProbe::stop()
{
    LOG("stop()",0);
    m_running = false;
}


//--------------------------------------
// update
//--------------------------------------

void AudioProbe::update(void)
{
    // always receive and release input blocks
    // we only re-buffer them if we are running

    s16 *ip[PROBE_CHANNELS];
	audio_block_t *in[PROBE_CHANNELS];
	for (u16 j=0; j<PROBE_CHANNELS; j++)
    {
		in[j] = receiveReadOnly(j);
        ip[j] = in[j] ? in[j]->data : 0;
    }
    
    if (m_running)
    {
        u16 i=0;
        while (m_running && i<AUDIO_BLOCK_SAMPLES)
        {
            for (u16 j=0; j<PROBE_CHANNELS; j++)
            {
                s16 v = 0;
                if (ip[j])
                {
                    v = *ip[j];
                    ip[j] += m_skip + 1;
                }                
                m_buffer[j][m_offset] = v;
            }
            
            m_offset++;
            if (m_offset >= PROBE_SAMPLES)
            {
                #if AUDIO_PROBE_ONE_SHOT
                    m_running = false;
                #else
                    m_offset = 0;
                #endif
            }

            i += m_skip + 1;
        }
    }
    
	for (u16 j=0; j<PROBE_CHANNELS; j++)
    {
        if (in[j])
            release(in[j]);
    }
}



