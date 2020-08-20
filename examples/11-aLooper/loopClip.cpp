#include "Looper.h"
#include <circle/logger.h>

#define log_name "lclip"


//-------------------------------
// static extern
//-------------------------------

// static
CString *getClipStateName(u16 clip_state)
{
	CString *msg = new CString();

    if (clip_state == CLIP_STATE_NONE)
    {
        msg->Append("NONE ");
    }
    else
    {
        if (clip_state & CLIP_STATE_RECORD_MAIN)
            msg->Append("R_MAIN ");
        if (clip_state & CLIP_STATE_RECORD_END)
            msg->Append("R_END ");
        if (clip_state & CLIP_STATE_RECORDED)
            msg->Append("R_DONE ");
        if (clip_state & CLIP_STATE_PLAY_MAIN)
            msg->Append("P_MAIN ");
        if (clip_state & CLIP_STATE_PLAY_END)
            msg->Append("P_END ");
        if (clip_state & CLIP_STATE_PENDING_RECORD)
            msg->Append("PEND_R ");
        if (clip_state & CLIP_STATE_PENDING_PLAY)
            msg->Append("PEND_P ");
    }

    return msg;
}



//-------------------------------
// loopClip
//-------------------------------

loopClip::loopClip(u16 clip_num, loopTrack* pTrack) :
    publicClip(pTrack->getTrackNum(),clip_num)
{
    m_pLoopTrack = pTrack;
    init();
}

loopClip::~loopClip()
{
    init();
    m_pLoopTrack = 0;
}

// virtual
void loopClip::init()
    // init() clears the clip and
    // MUST must be called on abort of recording
{
    LOG("clip(%d,%d)::init()",m_track_num,m_clip_num);
    publicClip::init();
    m_buffer = 0;
}



void loopClip::stopImmediate()
    // aborts the current recording, if any
    // or returns the play pointers to zero,
{
    if (m_state & (CLIP_STATE_RECORD_MAIN | CLIP_STATE_RECORD_END))
       init();
    else
    {
        m_play_block = 0;
        m_crossfade_block = 0;
        m_state |= CLIP_STATE_RECORDED;
            // maintain the recorded state
    }
}


void loopClip::startPlaying()
    // if working with the same track that was/is being recorded
    // startEndingRecording() must be called first!
{
    #if DEBUG_UPDATE
        LOG("clip(%d,%d)::startPlaying()",m_track_num,m_clip_num);
    #endif
    m_play_block = 0;
    m_crossfade_block = 0;
    m_state |= CLIP_STATE_PLAY_MAIN;
}



void loopClip::startRecording()
    // startRecording() MUST be called before recording
    // and has precedence over startPlaying().  if already
    // recording, startEndingRecording() MUST be called
    // on this clip before startRecording() or stopPlaying()
    // on the (new, next) clip.
{
    #if DEBUG_UPDATE
        LOG("clip(%d,%d)::startRecording()",m_track_num,m_clip_num);
    #endif
    m_play_block = 0;
    m_record_block = 0;
    m_crossfade_block = 0;
    m_num_blocks = 0;
    m_max_blocks = (pTheLoopBuffer->getFreeBlocks() / LOOPER_NUM_CHANNELS) - CROSSFADE_BLOCKS;
    m_buffer = pTheLoopBuffer->getBuffer();
    m_state |= CLIP_STATE_RECORD_MAIN;
    m_pLoopTrack->incDecNumUsedClips(1);

}


void loopClip::startEndingRecording()
    // begin process of ending recording the clip.
    // At this moment we commit() m_record_block + CROSSFADE_BLOCKS to the loopBuffer
    // and establish the (non crossfade) length of the clip
{
    #if DEBUG_UPDATE
        LOG("clip(%d,%d)::startEndingRecording()",m_track_num,m_clip_num);
    #endif
    m_num_blocks = m_record_block;
    m_max_blocks = m_record_block + CROSSFADE_BLOCKS;
    pTheLoopBuffer->commitBlocks(m_max_blocks * LOOPER_NUM_CHANNELS);
    m_state &= ~CLIP_STATE_RECORD_MAIN;
    m_state |= CLIP_STATE_RECORD_END;
    m_pLoopTrack->incDecNumRecordedClips(1);
}

void loopClip::finishRecording()
    // called after recording the crossfade out
    // to "finalize" the recording of this clip
{
    #if DEBUG_UPDATE
        LOG("clip(%d,%d)::finishRecording()",m_track_num,m_clip_num);
    #endif
    m_state &= ~CLIP_STATE_RECORD_END;
    m_state |= CLIP_STATE_RECORDED;
}



#if 0   // old implementation
    void loopClip::incRecBlock()
        // it should also be aware of the clip0 length
        // handle the transition to recording cross fad
        //
        // for now it just stops incrementing at max blocks
    {
        if (m_record_block < m_max_blocks)
            m_record_block++;
        else if (m_state & CLIP_STATE_RECORD_END)
            finishRecording();
    }
#endif



//----------------------------------------------
// update
//----------------------------------------------

void loopClip::update(audio_block_t *in[], audio_block_t *out[])
    // update with post-increments and wrapping
{
    s16 *rp = 0;
    s16 *pp_main = 0;
    s16 *pp_fade = 0;

    if (m_state & (CLIP_STATE_RECORD_MAIN | CLIP_STATE_RECORD_END))
        rp = getBlock(m_record_block);
    if (m_state & (CLIP_STATE_PLAY_MAIN))
        pp_main = getBlock(m_play_block);
    if (m_state & (CLIP_STATE_PLAY_END))
        pp_fade = getBlock(m_crossfade_block);

    for (int channel=0; channel<LOOPER_NUM_CHANNELS; channel++)
    {
        s16 *ip = in[channel]->data;
        s16 *op = out[channel]->data;

        for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++)
        {
            if (rp) *rp++ = *ip++;
            if (pp_main) *op += *pp_main++;
            if (pp_fade) *op += *pp_fade++;
            op++;
        }
    }

    // increment block pointers

    if (rp)
    {
        m_record_block++;

        // if RECORD_END, and m_record_block has reached the crossfade out,
        // we can finish the recording, and for now, stop the loop machine

        if ((m_state & CLIP_STATE_RECORD_END) &&
            m_record_block == m_num_blocks + CROSSFADE_BLOCKS)
        {
            finishRecording();
            pTheLoopMachine->setLoopState(LOOP_STATE_STOPPED);
        }
    }

    if (pp_fade)
    {
        m_crossfade_block++;
        if (m_crossfade_block == m_num_blocks + CROSSFADE_BLOCKS)
        {
            m_state &= ~CLIP_STATE_PLAY_END;
            m_crossfade_block = 0;
        }
    }
    if (pp_main)
    {
        m_play_block++;
        if (m_play_block == m_num_blocks)
        {
            m_state |= CLIP_STATE_PLAY_END;
            m_crossfade_block = m_play_block;
            m_play_block = 0;
        }
    }
}


