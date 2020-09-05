#include "Looper.h"
#include <circle/logger.h>

#define log_name "ltrack"


loopTrack::loopTrack(u16 track_num) : publicTrack(track_num)
{
    m_track_num = track_num;
    for (int i=0; i<LOOPER_NUM_LAYERS; i++)
    {
        m_clips[i] = new loopClip(i,this);
    }
    init();
}


loopTrack::~loopTrack()
{
    for (int i=0; i<LOOPER_NUM_LAYERS; i++)
    {
        delete m_clips[i];
        m_clips[i] = 0;
    }
}



void loopTrack::init()
{
    publicTrack::init();
    for (int i=0; i<LOOPER_NUM_LAYERS; i++)
        m_clips[i]->init();
}

void loopTrack::stopImmediate()
{
    // back out of recording clip ... should be callback in clip's stopImmediate method.

    if (m_clips[m_num_recorded_clips]->getClipState() && CLIP_STATE_RECORD_MAIN)
        m_num_used_clips--;

    for (int i=0; i<m_num_used_clips; i++)
    {
        m_clips[i]->stopImmediate();
    }
}


void loopTrack::update(audio_block_t *in[], audio_block_t *out[])
{
    // call update() on all active clips

    for (int i=0; i<m_num_used_clips; i++)
        m_clips[i]->update(in,out);
}





void loopTrack::incDecNumUsedClips(int inc)
{
    m_num_used_clips += inc;
    #if DEBUG_LOOPER_UPDATE
        LOG("track(%d) inDecNumUsedClips(%d)=%d",m_track_num,inc,m_num_used_clips);
    #endif
}
void loopTrack::incDecNumRecordedClips(int inc)
{
    m_num_recorded_clips += inc;
    #if DEBUG_LOOPER_UPDATE
        LOG("track(%d) incDecNumRecordedClips(%d)=%d",m_track_num,inc,m_num_recorded_clips);
    #endif
}


void loopTrack::updateState(u16 how_called, u16 loop_state, u16 pending_command)
    // we assume we are never asked to do anything illegal
{
    if (pending_command == LOOP_COMMAND_STOP_IMMEDIATE)
    {
        init();
    }

    // here we assume that if NONE the command will be RECORD
    // and if STOPPED it will be RECORD or PLAY

    else if (loop_state == LOOP_STATE_NONE ||
             loop_state == LOOP_STATE_STOPPED)
    {
        // if we are the selected track
        // start all recorded tracks playing
        // and possibly start recording a new one

        if (pending_command && m_selected)
        {
            #if 0
                LOG("track(%d) responding to command(%d,%s) in loop_state(%s)",
                    m_track_num,
                    how_called,
                    getLoopCommandName(pending_command),
                    getLoopStateName(loop_state));
            #endif

            for (int i=0; i<m_num_recorded_clips; i++)
            {
                m_clips[i]->startPlaying();
            }

            // and here we assume the selected clip will
            // be m_num_recorded_clips + 1

            if (pending_command == LOOP_COMMAND_RECORD) // clip0 will be empty
            {
                if (m_selected_clip_num == 0)
                    pTheLoopMachine->incDecNumUsedTracks(1);
                m_clips[m_selected_clip_num]->startRecording();
            }

            // we have handled the command
            // we become the current track
            // and start the loop machine running

            pTheLoopMachine->setPendingCommand(LOOP_COMMAND_NONE);
            pTheLoopMachine->setCurTrackNum(m_track_num);
            pTheLoopMachine->setLoopState(LOOP_STATE_RUNNING);

        }
    }

    // the one-time STOP command has been issued and the
    // looper is in LOOP_STATE_STOPPING.  If we are recording
    // the 0th clip in the track, it can transit to RECORD_END.

    else if (pending_command == LOOP_COMMAND_STOP)
    {
        loopClip *pClip0 = m_clips[0];
        u16 clip_state0 = pClip0->getClipState();

        if (clip_state0 & CLIP_STATE_RECORD_MAIN)
            pClip0->startEndingRecording();

    }

}
