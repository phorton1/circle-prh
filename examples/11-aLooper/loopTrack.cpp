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


void loopTrack::commit_recording()
{
    if (m_num_clips >= LOOPER_NUM_LAYERS)
    {
        LOG_ERROR("Attempt to commit a recording with m_num_clips(%d)",m_num_clips);
        return;
    }
    loopClip *pClip = m_clips[m_num_clips];
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
        

