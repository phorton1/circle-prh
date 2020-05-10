#include "Looper.h"
#include <circle/logger.h>
#include <circle/synchronize.h>

#define log_name "lmachine"



loopMachine::loopMachine() :
   AudioStream(LOOPER_MAX_NUM_INPUTS,LOOPER_MAX_NUM_OUTPUTS,inputQueueArray)
{
    LOG("ctor",0);
    m_pLoopBuffer = new loopBuffer();
    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        m_tracks[i] = new loopTrack(i,this);
    }
    init();
    LOG("looper ctor finished",0);
}


loopMachine::~loopMachine()
{
    LOG("dtor",0);
    if (m_pLoopBuffer)
        delete m_pLoopBuffer;
    m_pLoopBuffer = 0;
    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        if (m_tracks[i])
            delete  m_tracks[i];
        m_tracks[i] = 0;
    }
    LOG("dtor finished",0);
}


void loopMachine::init()
{
    m_state = 0;
    m_pending_state = 0;
    m_cur_track_num = 0;
    m_selected_track_num = 0;
    m_pRecordClip = 0;
    
    m_pLoopBuffer->init();
    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        m_tracks[i]->init();
    }
}


// static
const char *loopMachine::getLoopStateName(u16 state)
{
    if (state == LOOP_STATE_NONE)         return "NONE";
    if (state == LOOP_STATE_RECORDING)    return "RECORDING";
    if (state == LOOP_STATE_PLAYING)      return "PLAYING";
    if (state == LOOP_STATE_STOPPED)      return "STOPPED";
    return "UNKNOWN_STATE";
}



// static
const char *loopMachine::getCommandName(u16 state)
{
    if (state == LOOP_COMMAND_NONE)                 return "";                // function is disabled
    if (state == LOOP_COMMAND_CLEAR_ALL)            return "CLEAR";
    if (state == LOOP_COMMAND_STOP)                 return "STOP";
    if (state == LOOP_COMMAND_PLAY)                 return "PLAY";
    if (state == LOOP_COMMAND_RECORD)               return "REC";
    if (state == LOOP_COMMAND_SELECT_NEXT_TRACK)    return "TRACK";
    if (state == LOOP_COMMAND_SELECT_NEXT_CLIP)     return "CLIP";
    if (state == LOOP_COMMAND_STOP_IMMEDIATE)       return "STOP!";

    return "UNKNOWN_STATE";
}



// ONLY THE update() method (interrupt handler) sets the state,
// and duh, you cannot call LOG() from it and expect it to work
// correctly.  Took 3 hours to find the source of the "noise"
// in the program this time, so I am commenting this method out
// as a precaution
//
// void  loopMachine::setLooperState(u16 state)
// {
//     m_state = state;
//     // LOG("setLooperState(%s)",getLoopStateName(state));
// }


void  loopMachine::setPendingState(u16 state)
    // WHEREAS, it's ok to LOG the pending state change
    // because this is NOT called from update()
{
    m_pending_state = state;
    LOG("setPendingState(%s)",getLoopStateName(state));
}


u16 loopMachine::getNumUsedTracks()
    // tracks are 'used' if the 0th clip
    // has any recorded content, or is recording
{
    u16 i = 0;
    u16 retval = 0;
    while (i < LOOPER_NUM_TRACKS &&
           m_tracks[i]->getClip(0)->getClipState() > LOOP_CLIP_STATE_EMPTY)
    {
        retval++;
        i++;
    }
    return retval;
}


void loopMachine::selectTrack(u16 num)
    // will do nothing if the track number is not valid
    // in context of the current state of the machine
{
    // bool select_it = false;
    // u16 sel = m_selected_track_num + 1;
    // u16 num_used = getNumUsedTracks();
    // if (sel < num_used)
    //     select_it = true;
    // else if (sel < LOOPER_NUM_TRACKS &&
    //          sel == num_used &&
    //          
    
    m_selected_track_num = num;
}




void loopMachine::command(u16 command, u16 param /*=0*/)
{
    switch (command)
    {
        case LOOP_COMMAND_CLEAR_ALL :
            {
                init();
                break;
            }
        case LOOP_COMMAND_STOP :
            {
                if (m_state == LOOP_STATE_PLAYING ||
                    m_state == LOOP_STATE_RECORDING)
                {
                    setPendingState(LOOP_STATE_STOPPED);
                }
                else
                {
                    LOG_WARNING("NO NEED TO STOP (should be different function)",0);
                }
                break;
            }
        case LOOP_COMMAND_STOP_IMMEDIATE :
            {
                if (m_state == LOOP_STATE_PLAYING ||
                    m_state == LOOP_STATE_RECORDING)
                {
                    m_state = LOOP_STATE_STOPPED;
                    m_pending_state = 0;
                    if (m_pRecordClip)
                    {
                        if (!m_pRecordClip->getClipNum())
                            m_pRecordClip->commit();
                        else
                            m_pRecordClip->init();
                        m_pRecordClip = 0;
                    }
                    getCurTrack()->zeroClips();
                }
                else
                {
                    LOG_WARNING("NO NEED TO STOP_IMMEDIATE (should be different function)",0);
                }
                break;
            }
        case LOOP_COMMAND_RECORD  :
            {
                loopTrack *pSelTrack = getSelectedTrack();
                u16 record_clip_num = pSelTrack->getNumClips();
                if (record_clip_num >= LOOPER_NUM_LAYERS)
                {
                    LOG_ERROR("No more clips available on track %d (record should be disabled)",pSelTrack->getTrackNum());
                    return;
                }
                setPendingState(LOOP_STATE_RECORDING);
                break;
            }
        case LOOP_COMMAND_PLAY    :
            {
                loopTrack *pSelTrack = getSelectedTrack();
                u16 num_clips = pSelTrack->getNumClips();
                if (!num_clips)
                {
                    LOG_ERROR("No clips available on track %d (play should be disabled)",pSelTrack->getTrackNum());
                    return;
                }
                setPendingState(LOOP_STATE_PLAYING);
                break;
            }
        case LOOP_COMMAND_SELECT_NEXT_TRACK :
            {
                LOG("SELECT_NEXT_TRACK(%d)",m_selected_track_num);
                
                setPendingState(LOOP_STATE_NONE);

                m_selected_track_num++;
                if (m_selected_track_num > getNumUsedTracks() ||
                    m_selected_track_num == LOOPER_NUM_TRACKS)
                    m_selected_track_num = 0;
                    
                if (m_selected_track_num != m_cur_track_num &&
                    m_state != LOOP_STATE_PLAYING &&
                    m_state != LOOP_STATE_RECORDING)
                {
                    LOG("changing cur_track_num to %d",m_selected_track_num);
                    m_cur_track_num = m_selected_track_num;
                }
                break;
            }
        case LOOP_COMMAND_SELECT_NEXT_CLIP :
            {
                loopTrack *pTrack = getTrack(getSelectedTrackNum());
                u16 clip_num = pTrack->getSelectedClipNum();
                LOG("SELECT_NEXT_CLIP(%d)",clip_num);
                
                clip_num++;
                if (clip_num > pTrack->getNumClips() ||
                    clip_num == LOOPER_NUM_LAYERS)
                    clip_num = 0;
                pTrack->setSelectedClipNum(clip_num);
            }
        default:
            LOG_ERROR("unimplemented command(%d)",command);
            break;
            
    }
}




// virtual
void loopMachine::update(void)
{
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
    
    loopTrack *pCurTrack = getCurTrack();

    if (m_pending_state)
    {
        // determine if we should keep rolling ...
        // ALL are immediate if we are stopped or none
        // STOPS are are immediate if we are not recording,
        // everything else is immediate if we are recording the 0th clip
        // otherwise, finally, we wait till the base track is at zero
        
        bool doit = false;
        if (m_state == LOOP_STATE_NONE || m_state == LOOP_STATE_STOPPED)
            doit = true;
        // else if (m_state == LOOP_STATE_PLAYING && m_pending_state == LOOP_STATE_STOPPED)
        //     doit = true;
        else if (m_pRecordClip && !m_pRecordClip->getClipNum())
            doit = true;
        else if (!pCurTrack->getClip(0)->getCurBlock())
            doit = true;
        
        
        if (doit)
        {
            // DO NOT USE LOG in an interrupt routine and expect it to work correctly!
            // LOG("handling pending change to %d",m_pending_state);
        
            // if we are recording, commit it ...
            
            if (m_pRecordClip)
            {
                m_pRecordClip->commit();
                m_pRecordClip = 0;
            }
        
            // if the selected track is not the same as the current track,
            // change it
            
            if (m_selected_track_num != m_cur_track_num)
            {
                pCurTrack->zeroClips();
                m_cur_track_num = m_selected_track_num;
                pCurTrack = getCurTrack();
            }
            
            // if switching to recording, get the next available clip
            // and start it. It is assumed nobody has told us to do
            // anything illegal, like record on an unavailable clip
            // or track!
            
            if (m_pending_state == LOOP_STATE_RECORDING)
            {
                u16 record_clip_num = pCurTrack->getNumClips();
                m_pRecordClip = pCurTrack->getClip(record_clip_num);
                m_pRecordClip->start();
            }
            
            // change to the new state
            // DO NOT WRITE TO THE SERIAL PORT, duh!
            
            m_state = m_pending_state;
            m_pending_state = 0;
        }
    }
    

    if (m_state == LOOP_STATE_RECORDING)
    {
        s16 *op = m_pRecordClip->getBlockBuffer();
        
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
        
        m_pRecordClip->incCurBlock();
    }

    // clear the input if NO_THRU_LOOPER
    #if NO_THRU_LOOPER
        for (u16 j=0; j<LOOPER_MAX_NUM_INPUTS; j++)     // ASSUMED TO BE TWO FOR THE MOMENT
        {
            memset(ip[j],0,AUDIO_BLOCK_BYTES);
        }
    #endif
    
    
    // mix the existing clips into the output
    
    if (m_state == LOOP_STATE_PLAYING ||
        m_state == LOOP_STATE_RECORDING)
    {
        u16 num_clips = pCurTrack->getNumRecordedClips();
        for (int i=0; i<num_clips; i++)
        {
            loopClip *pClip = pCurTrack->getClip(i);    // get the playback clip
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

                #if NO_THRU_LOOPER
                    if (!i)
                        memset(tp,0,AUDIO_BLOCK_BYTES);
                #endif
                
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






