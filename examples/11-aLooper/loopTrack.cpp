#include "Looper.h"
#include <circle/logger.h>

#define log_name "ltrack"


// static
CString *getTrackStateName(u16 track_state)
{
	CString *msg = new CString();

    if (track_state == TRACK_STATE_EMPTY)
        msg->Append("EMPTY ");
    if (track_state & TRACK_STATE_RECORDING)
        msg->Append("RECORDING ");
    if (track_state & TRACK_STATE_PLAYING)
        msg->Append("PLAYING ");
    if (track_state & TRACK_STATE_STOPPED)
        msg->Append("STOPPED ");
    if (track_state & TRACK_STATE_PENDING_RECORD)
        msg->Append("PEND_RECORD ");
    if (track_state & TRACK_STATE_PENDING_PLAY)
        msg->Append("PEND_PLAY ");
    if (track_state & TRACK_STATE_PENDING_STOP)
        msg->Append("PEND_STOP ");
    return msg;
}




// virtual

int loopTrack::getTrackState()
{
    int state = 0;
    if (m_num_used_clips == 0)
        state |= TRACK_STATE_EMPTY;

    for (int i=0; i<m_num_used_clips; i++)
    {
        loopClip *pClip = m_clips[i];
        int clip_state = pClip->getClipState();
        if (clip_state & (CLIP_STATE_RECORD_IN | CLIP_STATE_RECORD_MAIN))
            state |= TRACK_STATE_RECORDING;
        if (clip_state & CLIP_STATE_PLAY_MAIN)
            state |= TRACK_STATE_PLAYING;
    }

    if (m_num_used_clips && !(state & (TRACK_STATE_RECORDING | TRACK_STATE_PLAYING)))
        state |= TRACK_STATE_STOPPED;

    u16 pending = pTheLoopMachine->getPendingCommand();
    u16 sel_num = pTheLoopMachine->getSelectedTrackNum();

    if (pending && m_track_num == sel_num)
    {
        if (pending == LOOP_COMMAND_STOP)
            state |= TRACK_STATE_PENDING_STOP;
        else if (pending == LOOP_COMMAND_RECORD)
            state |= TRACK_STATE_PENDING_RECORD;
        else if (pending == LOOP_COMMAND_PLAY)
            state |= TRACK_STATE_PENDING_PLAY;
    }
    return state;
}


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


void loopTrack::update(audio_block_t *in[], audio_block_t *out[])
{
    // call update() on all active clips

    for (int i=0; i<m_num_used_clips; i++)
        m_clips[i]->update(in,out);
}



void loopTrack::incDecNumUsedClips(int inc)
{
    m_num_used_clips += inc;
    LOOPER_LOG("track(%d) inDecNumUsedClips(%d)=%d",m_track_num,inc,m_num_used_clips);
}
void loopTrack::incDecNumRecordedClips(int inc)
{
    m_num_recorded_clips += inc;
    LOOPER_LOG("track(%d) incDecNumRecordedClips(%d)=%d",m_track_num,inc,m_num_recorded_clips);
}
void loopTrack::incDecRunning(int inc)
{
    m_num_running_clips += inc;
    LOOPER_LOG("track(%d) m_num_running_clips=%d",m_track_num,m_num_running_clips);
    pTheLoopMachine->incDecRunning(inc);

}



void loopTrack::stopImmediate()
{
    for (int i=0; i<m_num_used_clips; i++)
    {
        m_clips[i]->stopImmediate();
    }
    m_num_running_clips = 0;
}



void loopTrack::updateState(u16 cur_command)
{
    LOOPER_LOG("track(%d) updateState(%s)",m_track_num,getLoopCommandName(cur_command));

    if (cur_command == LOOP_COMMAND_STOP ||
        cur_command == LOOP_COMMAND_PLAY)
    {
        for (int i=0; i<m_num_used_clips; i++)
        {
            m_clips[i]->updateState(cur_command);
        }
    }
    else if (cur_command == LOOP_COMMAND_RECORD)
    {
        for (int i=0; i<m_num_used_clips; i++)
        {
            m_clips[i]->updateState(LOOP_COMMAND_PLAY);
                // the command play on the recording clip
                // *may* cause it to stop recording, and
                // increment m_num_recorded_clips
        }
        if (m_num_recorded_clips < LOOPER_NUM_LAYERS)
        {
            m_clips[m_num_recorded_clips]->updateState(LOOP_COMMAND_RECORD);
        }
    }
}
