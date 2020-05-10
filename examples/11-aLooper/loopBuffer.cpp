#include "Looper.h"
#include <circle/logger.h>
#include <circle/alloc.h>

#define log_name "lbuffer"


loopBuffer::loopBuffer(u32 size)
{
    m_top = 0;
    m_size = size;
    LOG("loopBuffer allocating %ld bytes",m_size);
    m_buffer = (int16_t *) malloc(m_size);
    assert(m_buffer);
}

loopBuffer::~loopBuffer()
{
    free(m_buffer);
    m_size = 0;
    m_top = 0;
}


