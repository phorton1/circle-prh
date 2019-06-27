#include "recorder.h"
#include <circle/logger.h>
#include <circle/alloc.h>


#define log_name "record"

// #define RECORD_CHANNELS         4
// #define RECORD_SECONDS          10
// #define RECORD_SAMPLE_RATE      44100
//     // we always fill an integral number of AUDIO_BLOCK_SAMPLES
//     // the seconds is approximate
// #define RECORD_BUFFER_BLOCKS    ((RECORD_SECONDS * RECORD_SAMPLE_RATE) / AUDIO_BLOCK_SAMPLES)
// #define RECORD_BUFFER_SAMPLES   (RECORD_BUFFER_BLOCKS * AUDIO_BLOCK_SAMPLES)
// #define RECORD_BUFFER_BYTES     (RECORD_BUFFER_SAMPLES * sizeof(s16))


//------------------------------------------
// AudioRecorder initialization, etc
//------------------------------------------


// static
int16_t AudioRecorder::m_buffer[RECORD_CHANNELS][RECORD_BUFFER_SAMPLES];


AudioRecorder::AudioRecorder() :
    AudioStream(RECORD_CHANNELS, inputQueueArray)
{
    m_cur_block    = 0;
    m_num_blocks   = 0;
    m_running      = 0;
    m_record_mask  = 0;
    m_play_mask    = 0xffff;
    
    clear();
}    


void AudioRecorder::begin()
{
}


void AudioRecorder::clear()
{
    m_running      = 0;
    m_cur_block    = 0;
    m_num_blocks   = 0;
    for (u16 i=0; i<RECORD_CHANNELS; i++)
    {
        memset(m_buffer[i],0,RECORD_BUFFER_BYTES);
    }
}

    
void AudioRecorder::start()
{
    LOG("start()",0);
    m_running = true;
}

void AudioRecorder::stop()
{
    LOG("stop()",0);
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
    u16 record_mask = m_record_mask;
    
	for (u16 j=0; j<RECORD_CHANNELS; j++)
    {
        in[j] = receiveReadOnly(j);

        // turn off record for any channels with no blocks

        if (!in[j] && (m_record_mask & mask(j)))
            record_mask &= ~mask(j);
        ip[j] = in[j] ? in[j]->data : 0;
    }

    // move the input data into the buffer
    
    if (m_running)
    {
        // if recording, capture the data to the buffer

        u32 offset = m_cur_block * AUDIO_BLOCK_SAMPLES;
        
        if (m_record_mask)
        {
            for (u32 i=0; i<AUDIO_BLOCK_SAMPLES; i++)
            {
                for (u16 j=0; j<RECORD_CHANNELS; j++)
                {
                    if (record_mask & mask(j))
                    {
                        m_buffer[j][offset+i] = *ip[j]++;
                    }                
                }
            }
        }
        
        // replace in buffers with play buffers
        
        for (u16 j=0; j<RECORD_CHANNELS; j++)
        {
            if (!(record_mask & mask(j)))
            {
                if (in[j])
                    release(in[j]);
                in[j] = allocate();
                memcpy(in[j]->data,&m_buffer[j][offset],AUDIO_BLOCK_BYTES);
            }                
        }

        // bump the block number and quit if out of memory
        
        m_cur_block++;
        m_cur_block++;
        if (m_cur_block > m_num_blocks)
            m_num_blocks = m_cur_block;
        if (m_cur_block >= RECORD_BUFFER_BLOCKS)
            m_running = false;
    }
    
    // transmit and release the blocks
    
	for (u16 j=0; j<RECORD_CHANNELS; j++)
    {
        if (in[j])
        {
    		transmit(in[j], j);
            release(in[j]);
        }
    }
}



