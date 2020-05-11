#include "Looper.h"
#include <circle/logger.h>
#include <circle/alloc.h>

#define log_name "lbuffer"


loopBuffer::loopBuffer(u32 size)
{
    m_top = 0;
    m_size = size;
    #if ALLOCATE_LOOPBUFFER_ON_FIRST_USE
        m_buffer = 0;
    #else
        #if 0
            extern unsigned long prhFreePtr;
            LOG("loopBuffer using prhFreePtr=%08X  mem=%dM",prhFreePtr,mem_get_size()/1000000);
            m_buffer = (int16_t *) prhFreePtr + 4 * MEGABYTE;
        #else
            LOG("loopBuffer allocating %ld bytes",m_size);
            m_buffer = (int16_t *) malloc(m_size);
            LOG("loopBuffer=0x%08X  mem=%dM",(u32) m_buffer,mem_get_size()/1000000);
        #endif
    #endif
}

loopBuffer::~loopBuffer()
{
    free(m_buffer);
    m_size = 0;
    m_top = 0;
}


#if ALLOCATE_LOOPBUFFER_ON_FIRST_USE

    s16 *loopBuffer::getBuffer()
    {
        if (!m_buffer)
        {
            // kludge ... allocate an extra one then delete it
            // I had some problem when the buffer was allocated at looper constrution,
            // causing some window objects to be in low memory, and some to be in very
            // high memory.  Dunno why.  So, I made the allocation dynamic at the first
            // call to getBuffer, which made the problem "go away" (hiding it for later)
                
            LOG("loopBuffer allocating %ld bytes",m_size);
            m_buffer = (int16_t *) malloc(m_size);
            LOG("loopBuffer=0x%08X  mem=%dM",(u32) m_buffer,mem_get_size()/1000000);
        }
        
        return &m_buffer[m_top];
    }

#endif