#include "looper.h"
#include <circle/logger.h>

#define log_name "lclip"


loopClip::loopClip(u16 clip_num, loopTrack* pTrack, u16 num_channels /* =2 */)
{
    m_clip_num = clip_num;
    m_pLoopTrack = pTrack;
    m_pLoopBuffer = pTrack->getLoopBuffer();
    m_num_channels = num_channels;
    init();
}

bool loopClip::isSelected()
{
    return m_pLoopTrack->getSelectedClipNum() == m_clip_num ? true : false;
}

void loopClip::commit()
{
    // NO NO NO!!
    // commit() is called from the loopMachine::update() method (interrupt handler)
    // and CANNOT be expected to LOG() to the serial port and continue working
    // correctly!  So, don't call LOG("commit %d",m_cur_block) !!!
    
    m_num_blocks = m_cur_block;         // has been incremented already
    m_pLoopBuffer->commitBlocks(m_num_blocks * m_num_channels);
    m_cur_block = 0;
}        

