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
