#include "looper.h"
#include <circle/logger.h>

#define log_name "ltrack"


loopTrack::loopTrack(u16 track_num, loopMachine *pLooper)
{
    m_track_num = track_num;
    m_pLooper = pLooper;
    m_pLoopBuffer = pLooper->getLoopBuffer();
    for (int i=0; i<LOOPER_NUM_LAYERS; i++)
    {
        m_clips[i] = new loopClip(i,this);
    }
    init();
}


loopTrack::~loopTrack()
{
    m_pLoopBuffer = 0;
    for (int i=0; i<LOOPER_NUM_LAYERS; i++)
    {
        delete m_clips[i];
        m_clips[i] = 0;
    }
}


bool loopTrack::isSelected()
{
    return m_track_num == m_pLooper->getSelectedTrackNum() ? true : false;
}


loopClip *loopTrack::getClip(u16 num)
{
    if (num >= LOOPER_NUM_LAYERS)
    {
        LOG_ERROR("Attempt to getClip(%d)",num);
        return m_clips[0];  // maybe no crash
    }
    return m_clips[num];
}



