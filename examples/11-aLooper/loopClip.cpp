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
        if (clip_state & CLIP_STATE_RECORD_IN)
            msg->Append("R_IN ");
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
    // LOG("clip(%d,%d)::init()",m_track_num,m_clip_num);
    publicClip::init();
    m_buffer = 0;
}


//--------------------------------
// atomic state changes
//--------------------------------

void loopClip::stopImmediate()
    // aborts the current recording, if any
    // or returns the play pointers to zero.
    // It is assumed that the loopMachine will stop the
    // m_running value (no dec takes place)
{
    if (m_state & (CLIP_STATE_RECORD_IN | CLIP_STATE_RECORD_MAIN | CLIP_STATE_RECORD_END))
    {
        // the track was 'used' by virtue of being recorded
        m_pLoopTrack->incDecNumUsedClips(-1);

        // and we even 'unrecord' it if it happens to be in that RECORD_END window
        if (m_state & CLIP_STATE_RECORD_END)
            m_pLoopTrack->incDecNumRecordedClips(-1);

        if (m_clip_num == 0)
            pTheLoopMachine->incDecNumUsedTracks(1);

        init();
    }
    else
    {
        m_play_block = 0;
        m_crossfade_start = 0;
        m_crossfade_offset = 0;
        m_state |= CLIP_STATE_RECORDED;
            // maintain the recorded state
    }
}


void loopClip::_startRecording()
    // startRecording() MUST be called before recording
    // and has precedence over startPlaying().  if already
    // recording, startEndingRecording() MUST be called
    // on this clip before startRecording() or stopPlaying()
    // on the (new, next) clip.
{
    LOOPER_LOG("clip(%d,%d)::startRecording()",m_track_num,m_clip_num);
    m_play_block = 0;
    m_record_block = 0;
    m_crossfade_start = 0;
    m_crossfade_offset = 0;
    m_num_blocks = 0;
    m_max_blocks = (pTheLoopBuffer->getFreeBlocks() / LOOPER_NUM_CHANNELS) - CROSSFADE_BLOCKS;
    m_buffer = pTheLoopBuffer->getBuffer();
    m_state |= CLIP_STATE_RECORD_IN;    // CLIP_STATE_RECORD_MAIN;
    m_pLoopTrack->incDecNumUsedClips(1);
    m_pLoopTrack->incDecRunning(1);
    if (m_clip_num == 0)
        pTheLoopMachine->incDecNumUsedTracks(1);
}


void loopClip::_startEndingRecording()
    // begin process of ending recording the clip.
    // At this moment we commit() m_record_block + CROSSFADE_BLOCKS to the loopBuffer
    // and establish the (non crossfade) length of the clip
{
    LOOPER_LOG("clip(%d,%d)::startEndingRecording()",m_track_num,m_clip_num);
    m_num_blocks = m_record_block;
    m_max_blocks = m_record_block + CROSSFADE_BLOCKS;
    pTheLoopBuffer->commitBlocks(m_max_blocks * LOOPER_NUM_CHANNELS);
    m_state &= ~CLIP_STATE_RECORD_MAIN;
    m_state |= CLIP_STATE_RECORD_END;
    m_pLoopTrack->incDecNumRecordedClips(1);
}

void loopClip::_finishRecording()
    // called after recording the crossfade out
    // to "finalize" the recording of this clip
{
    LOOPER_LOG("clip(%d,%d)::finishRecording()",m_track_num,m_clip_num);
    m_state &= ~CLIP_STATE_RECORD_END;
    m_state |= CLIP_STATE_RECORDED;
    m_pLoopTrack->incDecRunning(-1);
}



void loopClip::_startPlaying()
    // if working with the same track that was/is being recorded
    // startEndingRecording() must be called first!
{
    LOOPER_LOG("clip(%d,%d)::startPlaying()",m_track_num,m_clip_num);
    m_play_block = 0;
    m_crossfade_start = 0;
    m_crossfade_offset = 0;
    m_state |= CLIP_STATE_PLAY_MAIN;
    m_pLoopTrack->incDecRunning(1);  // loop running count
}




void loopClip::_startCrossFadeOut()
    // start a fade out and DO start playing the next time through
{
    LOOPER_LOG("clip(%d,%d)::startCrossFade",m_track_num,m_clip_num);

    m_state |= CLIP_STATE_PLAY_END;
        // retains PLAY_MAIN bit
    m_crossfade_start = m_play_block;
        // will be set invariantly at the loop point
        // set in the loop
    m_crossfade_offset = 0;
    m_play_block = 0;
    m_pLoopTrack->incDecRunning(1);
}


void loopClip::_startFadeOut()
    // start a fade out, but do NOT start playing next time through
    // so we decrement the 'loop' running count
    //
    // effectively 'cancel' the PLAY_MAIN bit, and start, or
    // continue fading out where we are ..
{
    LOOPER_LOG("clip(%d,%d)::_startFadeOut",m_track_num,m_clip_num);

    if (m_play_block)   // &&
        // !(m_state & CLIP_STATE_PLAY_END))
        m_crossfade_start = m_play_block;

    m_state &= ~CLIP_STATE_PLAY_MAIN;
    m_state |= CLIP_STATE_PLAY_END;
    m_crossfade_offset = 0;
    m_play_block = 0;
    m_pLoopTrack->incDecRunning(-1);
}


void loopClip::_endFadeOut()
    // end a fade out
{
    LOOPER_LOG("clip(%d,%d)::_endFadeOut",m_track_num,m_clip_num);
    m_state &= ~CLIP_STATE_PLAY_END;
    m_crossfade_start = 0;
    m_crossfade_offset = 0;
    m_pLoopTrack->incDecRunning(-1);
}





//----------------------------------------------
// update
//----------------------------------------------

#define FADE_BLOCK_INCREMENT (1.0/((float)CROSSFADE_BLOCKS))
    // percentage fade per block

#define FADE_SAMPLE_INCREMENT (FADE_BLOCK_INCREMENT/((float)AUDIO_BLOCK_SAMPLES))
    // percentage fade per sample


void loopClip::update(audio_block_t *in[], audio_block_t *out[])
    // update with post-increments and wrapping
{
    s16 *rp = 0;
    s16 *pp_main = 0;
    s16 *pp_fade = 0;

    if (m_state & (CLIP_STATE_RECORD_IN | CLIP_STATE_RECORD_MAIN | CLIP_STATE_RECORD_END))
        rp = getBlock(m_record_block);
    if (m_state & (CLIP_STATE_PLAY_MAIN))
        pp_main = getBlock(m_play_block);
    if (m_state & (CLIP_STATE_PLAY_END))
        pp_fade = getBlock(m_crossfade_start+m_crossfade_offset);

    bool fade_in = (pp_main && m_play_block < CROSSFADE_BLOCKS) ? 1 : 0;
    float sample_fade = FADE_SAMPLE_INCREMENT;

    for (int channel=0; channel<LOOPER_NUM_CHANNELS; channel++)
    {
        s16 *ip = in[channel]->data;
        s16 *op = out[channel]->data;

        float i_fade = 1.0;
        float o_fade = 1.0;

        if (fade_in)
            i_fade = ((float)m_play_block) * FADE_BLOCK_INCREMENT;
        if (pp_fade)
            o_fade = ((float)(CROSSFADE_BLOCKS-m_crossfade_offset)) * FADE_BLOCK_INCREMENT;

        for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++)
        {
            // add the unmodified input data to the recording

            if (rp) *rp++ = *ip++;

            // add the main input (subject to in-fading)

            if (pp_main)
            {
                if (fade_in)
                {
                    float val = *pp_main++;
                    val = val * i_fade;
                    *op += (s16) val;
                    i_fade += sample_fade;
               }
                else
                {
                    *op += *pp_main++;
                }
            }

            if (pp_fade)
            {
                float val = *pp_fade++;
                val = val * o_fade;
                *op += (s16) val;
                o_fade -= sample_fade;
            }

            op++;
        }
    }

    // increment block pointers
    // handling post increment state changes ....

    if (rp)
    {
        m_record_block++;

        // if we have recorded all of the FADE_IN, progress
        // from state RECORD_IN to state RECORD_MAIN

        if ((m_state & CLIP_STATE_RECORD_IN) &&
            (m_record_block >= CROSSFADE_BLOCKS))
        {
            m_state &= ~CLIP_STATE_RECORD_IN;
            m_state |= CLIP_STATE_RECORD_MAIN;
        }

        // if RECORD_END, and m_record_block has reached the crossfade out,
        // we change our state (but leave the loop machine to it's own fate)

        else if ((m_state & CLIP_STATE_RECORD_END) &&
            m_record_block == m_num_blocks + CROSSFADE_BLOCKS)
        {
            _finishRecording();
        }
    }

    if (pp_fade)
    {
        m_crossfade_offset++;
        if (m_crossfade_offset == CROSSFADE_BLOCKS)
            _endFadeOut();
    }
    if (pp_main)
    {
        m_play_block++;
        if (m_play_block == m_num_blocks)
        {
            _startCrossFadeOut();
            // note that this post increment starting of
            // the next loop can be canceled in the next
            // updateState() that takes place at the
            // track change.
        }
    }
}



//----------------------------------------------
// UPDATE STATE
//----------------------------------------------


void loopClip::updateState(u16 cur_command)
{
    LOOPER_LOG("clip(%d,%d) updateState(%s)",m_track_num,m_clip_num,getLoopCommandName(cur_command));

    if (cur_command == LOOP_COMMAND_STOP)
    {
        if (m_state & CLIP_STATE_RECORD_MAIN)
        {
            _startEndingRecording();
        }
        else if (m_state & CLIP_STATE_PLAY_MAIN)
        {
            _startFadeOut();
        }
    }
    else if (cur_command == LOOP_COMMAND_PLAY)
    {
        if (m_state & CLIP_STATE_RECORD_MAIN)
        {
            _startEndingRecording();
        }
        _startPlaying();
    }
    else if (cur_command == LOOP_COMMAND_RECORD)
    {
        _startRecording();
    }
}
