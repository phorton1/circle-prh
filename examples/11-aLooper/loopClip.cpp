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
        
        
void loopClip::commit()
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

