#include "Looper.h"
#include <circle/logger.h>
#include <circle/alloc.h>

#define log_name "loopbuf"



LoopBuffer::LoopBuffer(u32 size)
{
    m_top = 0;
    m_size = size;
    LOG("LoopBuffer allocating %ld bytes",m_size);
    m_buffer = (int16_t *) malloc(m_size);
    assert(m_buffer);
}

LoopBuffer::~LoopBuffer()
{
    free(m_buffer);
    m_size = 0;
    m_top = 0;
}


//----------------------------------------------

#undef log_name
#define log_name "loopclip"

LoopClip::LoopClip(u16 clip_num, LoopTrack* pTrack, u16 num_channels /* =2 */)
{
    m_clip_num = clip_num;
    m_pLoopTrack = pTrack;
    m_pLoopBuffer = pTrack->getLoopBuffer();
    m_num_channels = num_channels;
    init();
}
        
        
void LoopClip::commit()
{
    m_num_blocks = m_cur_block + 1;
    m_pLoopBuffer->commitBlocks(m_num_blocks * m_num_channels);
    
    // set first and last samples in block to 0

    #if 9
        s16 *p = getBlockBuffer();
        p += AUDIO_BLOCK_SAMPLES - 1;
        *p = 0;
        p += AUDIO_BLOCK_SAMPLES;
        *p = 0;
    #endif
    
    m_cur_block = 0;

    #if 0
        p = getBlockBuffer();
        *p = 0;
        p += AUDIO_BLOCK_SAMPLES;
        *p = 0;
    #endif
    
}        


//----------------------------------------------

#undef log_name
#define log_name "looptrack"


LoopTrack::LoopTrack(u16 track_num, Looper *pLooper)
{
    m_track_num = track_num;
    m_pLooper = pLooper;
    m_pLoopBuffer = pLooper->getLoopBuffer();
    for (int i=0; i<LOOPER_NUM_LAYERS; i++)
    {
        m_clips[i] = new LoopClip(i,this);
    }
    init();
}


LoopTrack::~LoopTrack()
{
    m_pLoopBuffer = 0;
    for (int i=0; i<LOOPER_NUM_LAYERS; i++)
    {
        delete m_clips[i];
        m_clips[i] = 0;
    }
}


bool LoopTrack::isSelected()
{
    return m_track_num == m_pLooper->getSelectedTrackNum() ? true : false;
}


LoopClip *LoopTrack::getClip(u16 num)
{
    if (num >= LOOPER_NUM_LAYERS)
    {
        LOG_ERROR("Attempt to getClip(%d)",num);
        return m_clips[0];  // maybe no crash
    }
    return m_clips[num];
}


void LoopTrack::commit_recording()
{
    if (m_num_clips >= LOOPER_NUM_LAYERS)
    {
        LOG_ERROR("Attempt to commit a recording with m_num_clips(%d)",m_num_clips);
        return;
    }
    LoopClip *pClip = m_clips[m_num_clips];
    u32 blocks = pClip->getCurBlock();
    if (!blocks)
    {
        LOG_ERROR("Attempt to commit an empty recording",0);
        return;
    }
    if (pClip->getNumBlocks())
    {
        LOG_ERROR("Attempt to commit over an already recorded track",0);
        return;
    }
    // LOG("committing recording",0);
    pClip->commit();
    #if 0
        m_base_block_length ||= blocks;
        if (blocks > m_max_block_length)
            m_max_block_length = blocks;
    #endif
    m_num_clips++;
}
        


//----------------------------------------------

#undef log_name
#define log_name "looper"


const char *Looper::getLooperStateName(u16 state)
{
    if (state == LOOP_STATE_NONE)         return "NONE";
    if (state == LOOP_STATE_RECORDING)    return "RECORDING";
    if (state == LOOP_STATE_PLAYING)      return "PLAYING";
    if (state == LOOP_STATE_STOPPED)      return "STOPPED";
    return "UNKNOWN_STATE";
}


void  Looper::setLooperState(u16 state)
{
    m_state = state;
    LOG("setLooperState(%s)",getLooperStateName(state));
}

void  Looper::setPendingState(u16 state)
{
    m_pending_state = state;
    LOG("setPendingState(%s)",getLooperStateName(state));
}



Looper::Looper() :
   AudioStream(LOOPER_MAX_NUM_INPUTS,LOOPER_MAX_NUM_OUTPUTS,inputQueueArray)
{
    LOG("Looper ctor",0);
    m_pLoopBuffer = new LoopBuffer();
    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        m_tracks[i] = new LoopTrack(i,this);
    }
    init();
    LOG("Looper ctor finished",0);
}


Looper::~Looper()
{
    LOG("ctor",0);
    delete m_pLoopBuffer;
    m_pLoopBuffer = 0;
    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        delete  m_tracks[i];
        m_tracks[i] = 0;
    }
    LOG("ctor finished",0);
}


void Looper::init()
{
    setLooperState(LOOP_STATE_NONE); 

    m_pending_state = 0;
    
    m_num_used_tracks = 0;
    m_cur_track_num = 0;
    m_selected_track_num = 0;
    
    m_pLoopBuffer->init();
    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        m_tracks[i]->init();
    }
}




void Looper::command(u16 command, u16 param /*=0*/)
{
    LoopTrack *pCurTrack = getCurTrack();
    u16 clip_num = pCurTrack->getNumClips();
    
    switch (command)
    {
        case LOOP_COMMAND_CLEAR_ALL :
            {
                init();
                break;
            }
        case LOOP_COMMAND_STOP :
            {
                if (m_state == LOOP_STATE_PLAYING)
                {
                    setLooperState(LOOP_STATE_STOPPED);
                    pCurTrack->zeroClips();                 // move all clips back to zero
                }
                else if (m_state == LOOP_STATE_RECORDING)
                {
                    if (!clip_num)                      // base track finishes recording
                    {
                        setLooperState(LOOP_STATE_STOPPED);
                        LOG("Committing recording",0);
                        pCurTrack->commit_recording();
                        pCurTrack->zeroClips();                 // move all clips back to zero
                    }
                    else if (0)                               // subsequent tracks abort recording on STOP
                    {
                        setLooperState(LOOP_STATE_STOPPED);
                        LOG("Aborting recording",0);
                        pCurTrack->getClip(clip_num)->init();
                        pCurTrack->zeroClips();                 // move all clips back to zero
                    }
                    else
                    {
                        LOG("STOP Finishing pending recording",0);
                        setPendingState(LOOP_STATE_STOPPED);
                    }
                }
                else
                {
                    LOG_WARNING("NO NEED TO STOP (should be different function)",0);
                }
                break;
            }
        case LOOP_COMMAND_RECORD  :
            {
                if (clip_num >= LOOPER_NUM_LAYERS)
                {
                    LOG_ERROR("No more clips available on this track (record should be disabled)",0);
                    return;
                }
                if (m_state == LOOP_STATE_NONE ||
                    m_state == LOOP_STATE_STOPPED)
                {
                    LOG("recording onto clip %d",clip_num);
                    pCurTrack->getClip(clip_num)->start();
                    setLooperState(LOOP_STATE_RECORDING);
                }
                else if (m_state == LOOP_STATE_PLAYING)
                {
                    pCurTrack->getClip(clip_num)->start();
                    setPendingState(LOOP_STATE_RECORDING);
                }
                else if (m_state == LOOP_STATE_RECORDING)
                {
                    // on the 0th clip, we finish the recording, then start recording another
                    // on subsequent clips, we do this in a pending manner

                    if (!clip_num)                      // base track finishes recording
                    {
                        setLooperState(LOOP_STATE_STOPPED);
                        pCurTrack->commit_recording();
                        clip_num = pCurTrack->getNumClips();
                        pCurTrack->getClip(clip_num)->start();
                        setLooperState(LOOP_STATE_RECORDING);
                    }
                    else                                // subsequent tracks abort recording
                    {
                        if (clip_num >= LOOPER_NUM_LAYERS-1)
                        {
                            LOG_ERROR("No more pending clips available on this track (record should be disabled)",0);
                            return;
                        }
                        setPendingState(LOOP_STATE_RECORDING);
                    }
                }
                break;
            }
        case LOOP_COMMAND_PLAY    :
            {
                if (m_state == LOOP_STATE_RECORDING)
                {
                    // on the 0th clip, we finish the recording, then start playing
                    // on subsequent clips, we do this in a pending manner

                    if (!clip_num)                      // base track finishes recording
                    {
                        setLooperState(LOOP_STATE_STOPPED);
                        pCurTrack->commit_recording();
                        setLooperState(LOOP_STATE_PLAYING);
                    }
                    else                                // subsequent tracks abort recording
                    {
                        setPendingState(LOOP_STATE_PLAYING);
                    }
                }
                
                // othrwise, the play button is a toggle, stopping and starting at the given position

                else if (m_state == LOOP_STATE_PLAYING)
                {
                    setLooperState(LOOP_STATE_STOPPED);
                }
                else if (pCurTrack->getNumClips())
                {
                    setLooperState(LOOP_STATE_PLAYING);
                }
                else
                {
                    LOG_ERROR("NO CLIPS TO PLAY",0);
                }
                break;
            }
        default:
            LOG_ERROR("unimplemented command(%d)",command);
            break;
            
    }
}




// virtual
void Looper::update(void)
{
    LoopTrack *pCurTrack = getCurTrack();
    u16 num_clips = pCurTrack->getNumClips();
    
    // always receive any input blocks
    // which in our case will always have content
    
    s16 *ip[LOOPER_MAX_NUM_INPUTS];
	audio_block_t *in[LOOPER_MAX_NUM_INPUTS];
	for (u16 j=0; j<LOOPER_MAX_NUM_INPUTS; j++)
    {
        in[j] = receiveWritable(j);
        ip[j] = in[j] ? in[j]->data : 0;
        // assert(ip[j]);   FUCKING CRASHES IF YOU ASSERT HERE, FFS
    }

    
    if (m_pending_state)
    {
        LoopClip *pBaseClip = pCurTrack->getClip(0);
        if (!pBaseClip->getCurBlock())          // it just wrapped
        {
            if (m_state == LOOP_STATE_RECORDING)
            {
                pCurTrack->commit_recording();
            }
            setLooperState(m_pending_state);
            m_pending_state = 0;
            num_clips = pCurTrack->getNumClips();
            if (m_state == LOOP_STATE_RECORDING)
            {
                pCurTrack->getClip(num_clips)->start();
            }
        }
    }
    
            
    if (m_state == LOOP_STATE_RECORDING)
    {
        LoopClip *pClip = pCurTrack->getClip(num_clips);    // get the recording clip
        s16 *op = pClip->getBlockBuffer();
        if (!op)
        {
            LOG_ERROR("LOOPER BUFFER OVERFLOW",0);
            init();
            return;
        }
        
        for (u16 j=0; j<LOOPER_MAX_NUM_INPUTS; j++)     // ASSUMED TO BE TWO FOR THE MOMENT
        {
            memcpy(op,ip[j],AUDIO_BLOCK_BYTES);
            op += AUDIO_BLOCK_SAMPLES;
        }
        
        pClip->incCurBlock();
    }
    
    // mix the existing clips into the output
    
    if (m_state == LOOP_STATE_PLAYING ||
        m_state == LOOP_STATE_RECORDING)
    {
        for (int i=0; i<num_clips; i++)
        {
            LoopClip *pClip = pCurTrack->getClip(i);    // get the playback clip
            s16 *op = pClip->getBlockBuffer();

            // on the first and last blocks of playback
            // ramp the sound in and out ..
            
            #define FADE_IN_BLOCKS   0     // approx 15 ms
            
            #if FADE_IN_BLOCKS
                u32 cur_block = pClip->getCurBlock();
                u32 num_blocks = pClip->getNumBlocks();
                double scale = FADE_IN_BLOCKS * AUDIO_BLOCK_SAMPLES - 1;
                u32 inv_block = num_blocks - cur_block - 1;     // FADE_IN_BLOCKS-1..0
                //if (cur_block >= num_blocks - FADE_IN_BLOCKS)
                //{
                //    LOG("%d %d %d",num_blocks,cur_block,inv_block);
                //}
            #endif
            
            // mix playing clips into audio buffers
            
            for (u16 j=0; j<LOOPER_MAX_NUM_INPUTS; j++)     // ASSUMED TO BE TWO FOR THE MOMENT
            {
                s16 *tp = ip[j];
                
                for (u16 i=0; i<AUDIO_BLOCK_SAMPLES; i++)
                {
                    s16 val = op[i];

                    #if FADE_IN_BLOCKS
                        if (cur_block < FADE_IN_BLOCKS)                     // 0..FADE_IN_BLOCKS=1
                        {
                            double pos = cur_block * AUDIO_BLOCK_SAMPLES + i;
                            
                            double fval = val;
                            fval *= pos/scale;
                            val = fval;
                        }
                        else if (cur_block >= num_blocks - FADE_IN_BLOCKS)
                        {
                            u16 inv_i = AUDIO_BLOCK_SAMPLES - i - 1;
                            double pos = inv_block * AUDIO_BLOCK_SAMPLES + inv_i;
    
                            double fval = val;
                            fval *= pos/scale;
                            val = fval;
                        }
                    #endif
                    
                    *tp++ += val;
                }
                op += AUDIO_BLOCK_SAMPLES;
            }
            
            pClip->incCurBlock();
        }
    }
    
    // transmit the output blocks
    
	for (u16 j=0; j<LOOPER_MAX_NUM_INPUTS; j++)
    {
        if (in[j])      // always true right now
        {
            transmit(in[j], j);
            AudioSystem::release(in[j]);
        }
    }
}






