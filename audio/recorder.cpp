#include "recorder.h"
#include <circle/logger.h>
#include <circle/alloc.h>


#define log_name "record"


//------------------------------------------
// AudioRecorder initialization, etc
//------------------------------------------


AudioRecorder::AudioRecorder() :
    AudioStream(RECORD_CHANNELS,RECORD_CHANNELS,inputQueueArray)
{
    m_cur_block    = 0;
    m_num_blocks   = 0;
    m_running      = 0;
    m_record_mask  = 0;
    m_play_mask    = 0xffff;
    for (int i=0; i<RECORD_CHANNELS; i++)
        m_buffer[i] = 0;
}    


void AudioRecorder::start()
{
    for (int i=0; i<RECORD_CHANNELS; i++)
    {
        if (!m_buffer[i])
        {
            m_buffer[i] = (int16_t *) malloc(RECORD_BUFFER_BYTES);
            assert(m_buffer[i]);
        }
    }
    clearRecording();
}


void AudioRecorder::clearRecording()
{
    m_running      = 0;
    m_cur_block    = 0;
    m_num_blocks   = 0;
    for (u16 i=0; i<RECORD_CHANNELS; i++)
    {
        if (m_buffer[i])
            memset(m_buffer[i],0,RECORD_BUFFER_BYTES);
    }
}

    
void AudioRecorder::startRecording()
{
    LOG("startRecording()",0);
    m_cur_block = 0;
    m_running = true;
}

void AudioRecorder::stopRecording()
{
    LOG("stopRecording()",0);
    m_running = false;
}


//--------------------------------------
// update
//--------------------------------------

#define mask(j) (1<<j)

void AudioRecorder::update(void)
{
    // always receive any input blocks

    s16 *ip[RECORD_CHANNELS];
	audio_block_t *in[RECORD_CHANNELS];
    
	for (u16 j=0; j<RECORD_CHANNELS; j++)
    {
        in[j] = receiveReadOnly(j);
        ip[j] = in[j] ? in[j]->data : 0;
    }

    // if running, check 0th buffer jic we are called before begin() somehow
    
    if (m_running && m_buffer[0])   
    {
        // if recording, capture the data to the buffer

        u32 offset = m_cur_block * AUDIO_BLOCK_SAMPLES;
        
        if (m_record_mask)
        {
            for (u32 i=0; i<AUDIO_BLOCK_SAMPLES; i++)
            {
                for (u16 j=0; j<RECORD_CHANNELS; j++)
                {
                    if (m_record_mask & mask(j))
                    {
                        m_buffer[j][offset+i] =
                            ip ? *ip[j]++ : 0;
                    }                
                }
            }
        }
        
        // replace any other playing channels with previously recorded data
        
        for (u16 j=0; j<RECORD_CHANNELS; j++)
        {
            if (m_play_mask & mask(j) &&
                !(m_record_mask & mask(j)))
            {
                if (in[j])
                    AudioSystem::release(in[j]);
                in[j] = AudioSystem::allocate();
                memcpy(in[j]->data,&m_buffer[j][offset],AUDIO_BLOCK_BYTES);
            }                
        }

        // bump the block number and quit if out of memory
        
        m_cur_block++;
        if (m_cur_block > m_num_blocks)
            m_num_blocks = m_cur_block;
        if (m_cur_block >= RECORD_BUFFER_BLOCKS)
            m_running = false;
    }
    
    // transmit and playing channels and release the blocks
    
	for (u16 j=0; j<RECORD_CHANNELS; j++)
    {
        if (in[j])
        {
            if (m_play_mask & mask(j))            
                transmit(in[j], j);
            AudioSystem::release(in[j]);
        }
    }
}



