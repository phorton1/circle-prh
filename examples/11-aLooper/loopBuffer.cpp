#include "Looper.h"
#include <circle/logger.h>
#include <circle/alloc.h>

#define log_name "lbuffer"

loopBuffer *pTheLoopBuffer = 0;

loopBuffer::loopBuffer(u32 size)
{
    pTheLoopBuffer = this;

    m_top = 0;
    m_size = size;
    LOG("loopBuffer allocating %ld bytes",m_size);
    m_buffer = (int16_t *) malloc(m_size);
    LOG("loopBuffer=0x%08X  mem=%dM",(u32) m_buffer,mem_get_size()/1000000);
}

loopBuffer::~loopBuffer()
{
    free(m_buffer);
    m_size = 0;
    m_top = 0;
}
