
#include "Audio.h"
#include <circle/logger.h>
#include <circle/alloc.h>

#define log_name "audio"

#define AUDIO_RESERVE_MEMORY  4000000
    // reserve 4MB 


AudioCodec *AudioCodec::s_pCodec = 0;

u16  AudioSystem::s_numStreams = 0;
u32  AudioSystem::s_totalBlocks = 0;
u32  AudioSystem::s_blocksUsed = 0;
u32  AudioSystem::s_blocksUsedMax = 0;
u32  AudioSystem::s_cpuCycles = 0;
u32  AudioSystem::s_nInUpdate = 0;
u32  AudioSystem::s_cpuCyclesMax = 0;
u32  AudioSystem::s_numOverflows = 0;
bool AudioSystem::s_bUpdateScheduled = 0;

AudioStream   *AudioSystem::s_pFirstStream = 0;
AudioStream   *AudioSystem::s_pLastStream = 0;
audio_block_t *AudioSystem::s_pAudioMemory = 0;
audio_block_t *AudioSystem::s_pFreeBlock = 0;



void AudioSystem::start()
{
}


void AudioSystem::stop()
{
	// PRH TODO: stop and restart audioUpdateTask, objects in general
	s_bUpdateScheduled = false;
}



bool AudioSystem::initialize(u32 num_audio_blocks)
{
    LOG("initialize(%d)",num_audio_blocks);
    
    if (!initialize_memory(num_audio_blocks))
        return false;

	// sort the streams
    
    sortStreams();
		
    // start the codec if there's one
    
    if (AudioCodec::s_pCodec)
    {
        LOG("Starting %s",AudioCodec::s_pCodec->getName());
        AudioCodec::s_pCodec->start();
    }

	// call start() on each AudioStream()
	// note that oodecs usually must be started before streams

	for (AudioStream *p = s_pFirstStream; p; p = p->m_pNextStream)
	{
        LOG("Starting %s[%d]",p->getName(),p->getInstance());
		p->start();
	}

    return true;

}   // AudioSystem::initialize()



void AudioSystem::AddStream(AudioStream *pStream)
{
    if (s_pLastStream)
        s_pLastStream->m_pNextStream = pStream;
    else
        s_pFirstStream = pStream;
    s_pLastStream = pStream;
    s_numStreams++;    
}


void AudioSystem::resetStats()
{
	s_cpuCycles     = 0;
	s_cpuCyclesMax  = 0;
	s_numOverflows  = 0;
    
    for (AudioStream *p=s_pFirstStream; p; p=p->m_pNextStream)
        p->resetStats();
}


AudioStream *AudioSystem::find(u32 type, const char *name, s16 instance)
{
    for (AudioStream *p=s_pFirstStream; p; p=p->m_pNextStream)
    {
        bool type_match = (!type) || (type == p->getType());
        bool name_match = (!name) || (!strcmp(name,p->getName()));
        bool inst_match = (instance == -1) || (instance == (s16) p->getInstance());
        if (type_match && name_match && inst_match)
        {
            return p;            
        }
    }
    return NULL;
}


//----------------------------------------
// memory
//----------------------------------------


bool AudioSystem::initialize_memory(u32 num_audio_blocks)
{
    LOG("initialize_memory(%d)",num_audio_blocks);
    u32 bytes = num_audio_blocks * sizeof(audio_block_t);
    u32 avail = mem_get_size() - AUDIO_RESERVE_MEMORY;
    if (bytes > avail)
    {
        LOG_ERROR("cannot allocate %d memory blocks (%d bytes) max=%d bytes",
            num_audio_blocks,bytes,avail);
        return false;
    }
    
    s_pAudioMemory = (audio_block_t *) malloc(bytes);
    assert(s_pAudioMemory);
    if (!s_pAudioMemory)
    {
        LOG_ERROR("could not allocate %d memory blocks (%d bytes) max=%d bytes",
            num_audio_blocks,bytes,avail);
        return false;
    }

    // setup the free list
    
    s_totalBlocks = num_audio_blocks;
    audio_block_t *p = s_pAudioMemory;
    audio_block_t *prev = 0;
    
    for (u32 i=0; i<s_totalBlocks; i++)
    {
        p->next = 0;
        if (prev) prev->next = p;
        prev = p;
        p++;
    }
    
    s_pFreeBlock = s_pAudioMemory;
    return true;
}


// volatile bool show_allocs = 0;

audio_block_t *AudioSystem::allocate(void)
{
    // if (show_allocs)
    // {
    //     LOG("alloc   %d/%d  %08lx", s_blocksUsed,s_totalBlocks,(u32)s_pFreeBlock);
    //     delay(5);
    // }

	__disable_irq();

    if (!s_pFreeBlock)
    {
        static bool out_of_memory_error = 0;
        if (!out_of_memory_error)
            LOG_ERROR("OUT OF MEMORY",0);
        out_of_memory_error = 1;
    	__enable_irq();
        return NULL;
    }
    
    audio_block_t *block = s_pFreeBlock;
    s_pFreeBlock = block->next;
    s_pFreeBlock->prev = 0;

    block->prev = 0;
    block->next = 0;
   	block->ref_count = 1;

    s_blocksUsed++;
    if (s_blocksUsed > s_blocksUsedMax)
        s_blocksUsedMax = s_blocksUsed;
        
	__enable_irq();
    return block;
}	


void AudioSystem::release(audio_block_t *block)
{
    // assert(block);
		// prh 2025-03-10 was getting this assert at startup every time
		// so I removed it.
    if (!block)
        return;
    
	__disable_irq();
    
    if ( ((u32)block) < ((u32)s_pAudioMemory) ||
         ((u32)block) > ((u32)s_pAudioMemory) + ((u32)s_totalBlocks*AUDIO_BLOCK_BYTES) )
    {
        static bool bad_pointer_error = 0;
        if (!bad_pointer_error)
            LOG_ERROR("release BAD POINTER block(%08x) mem=(%08x to %08x)",
                (u32) block,
                (u32)s_pAudioMemory,
                ((u32)s_pAudioMemory) + ((u32)s_totalBlocks*AUDIO_BLOCK_BYTES));
        bad_pointer_error = 1;
    	__enable_irq();
        return;
    }
    
	if (block->ref_count > 1)
	{
		block->ref_count--;
	}
	else
    {    
        // if (show_allocs)
        // {
        //     LOG("release %d/%d  %08lx free=%08lx", s_blocksUsed,s_totalBlocks,(u32)block,(u32)s_pFreeBlock);
        //     delay(5);
        // }

        block->prev = 0;
        block->next = (audio_block_t *) s_pFreeBlock;
        if (s_pFreeBlock)
            s_pFreeBlock->prev = block;
        s_pFreeBlock = block;
        s_blocksUsed--;
    }
    
	__enable_irq();
}	


//----------------------------------------------
// update sorting
//----------------------------------------------


#define UPDATE_DEPTH_LIMIT  255
    // for detecting circularity
    // null nodes will be UPDATE_DEPTH_LIMIT+1
    // disconnected nodes will be UPDATE_DEPTH_LIMIT+2

    
void AudioSystem::traverse_update(u16 depth, AudioStream *p)
{
    if (!(p->getType() & AUDIO_DEVICE_INPUT))
        depth += 3;
    if (!(p->getType() & AUDIO_DEVICE_OUTPUT))
        depth += 3;
        
    if (depth >= UPDATE_DEPTH_LIMIT)
    {
        p->m_updateDepth = depth;				
        LOG_ERROR("update depth limit(%d) reached on %s%i (circular reference)",
            depth, p->getName(), p->getInstance());
        return;
    }
    if (depth > p->m_updateDepth)
        p->m_updateDepth = depth;
        
    #if 0
		CString fill;
        for (u16 i=0; i<depth; i++)
            fill.Append("  ");
		LOG("%s%s%d   depth:%d   max:%d",(const char *)fill,p->getName(),p->getInstance(),depth,p->m_updateDepth);
    #endif
    
    for (AudioConnection *con=p->m_pFirstConnection; con; con=con->m_pNextConnection)
    {
        traverse_update(depth+1,&con->m_dest);
    }
}


void AudioSystem::sortStreams()
    // Topologically sorting the updates is trivial if it is a tree.
    // If it is a graph (has cycles), it is not so easy.
    //
    // If TOPOLOGICAL_SORT is not defined, then you should declare
    // the AudioStream objects in the order you want the updates to occur,
    // i.e. declare inputs first, then effects, etc, then outputs.
    //
    // Our first approximation is a simple depth first tree traversal
    // with a limit to detect circularity. First we determine the "root"
    // input nodes and we traverse them, assigning the maximum depth of
    // each node visited.  NULL nodes (no inputs or outputs) can exists,
    // think of an i2sdevice that just outputs sound. It has no connections.
    // NULL nodes are placed after any visited nodes.
    //
    // Finally there may be dangling nodes, that have inputs and/or outputs
    // but, for some reason, were not visited during the traversal.  These
    // are just added last, then the list is sorted, then rebuilt.
{
    LOG("topologically sorting %d audio streams ...",s_numStreams);
    
    if (!s_numStreams)
    {
        LOG_ERROR("No AudioStreams found!!",0);
        return;
    }

	// initialize pUpdateDepth
    for (AudioStream *p = s_pFirstStream; p; p = p->m_pNextStream)
    {
		p->m_updateDepth = 0;
	}
	
	// traverse output only audio devices
    
    u16 obj_num = 0;
    AudioStream *objs[s_numStreams];
    for (AudioStream *p = s_pFirstStream; p; p = p->m_pNextStream)
    {
        objs[obj_num++] = p;
        if (!p->getNumInputs())
        {
            if (p->m_pFirstConnection)
            {
				// LOG("    traversing %s:%d start=%d",p->getName(),p->getInstance(),p->m_updateDepth);
                traverse_update(1,p);
            }
            else
            {
                LOG_WARNING("null update node: %s%d",p->getName(),p->getInstance());
                p->m_updateDepth = UPDATE_DEPTH_LIMIT + 1;
            }
        }
    }
    
    // mark dangling nodes
    
    for (u16 i=0; i<s_numStreams; i++)
    {
        AudioStream *p = objs[i];
        if (!p->m_updateDepth)
        {
            LOG_WARNING("dangling update node: %s%d",p->getName(),p->getInstance());
            p->m_updateDepth = UPDATE_DEPTH_LIMIT + 2;
        }
        p->m_pNextStream = 0;
    }
    
    // sort em
    
    u16 i = 0;
    while (i < (unsigned int) s_numStreams-1)
    {
        if (objs[i]->m_updateDepth > objs[i+1]->m_updateDepth)
        {
            AudioStream *tmp = objs[i];
            objs[i] = objs[i+1];
            objs[i+1] = tmp;
            if (i) i--;
        }
        else
        {
            i++;
        }
    }

    // rebuild the list

    s_pFirstStream = objs[0];
    AudioStream *prev = objs[0];
    for (i=1; i<s_numStreams; i++)
    {
		#if 0
			LOG("%d    %s:%d --> %s:%d",
				prev->m_updateDepth,
				prev->getName(),prev->getInstance(),
				objs[i]->getName(),objs[i]->getInstance());
		#endif
		
        prev->m_pNextStream = objs[i];
        prev = objs[i];
    }

    s_pLastStream = prev;
    // LOG("topo sort finished",0);
}


//----------------------------------------------
// update
//----------------------------------------------

bool AudioSystem::takeUpdateResponsibility()
{
	if (s_bUpdateScheduled)
		return false;
	s_bUpdateScheduled = true;
	return true;
    
}


void AudioSystem::startUpdate()
{
	if (s_nInUpdate++)
		s_numOverflows++;

	// If single core we call do_update() directly from the audio IRQ.
	// If multi-core, the IRQ is running on core0 and we send an IPI
	// to the given core which will then call do_update()
	
	#if CORE_FOR_AUDIO_SYSTEM == 0
		doUpdate();
	#else
		CCoreTask::Get()->SendIPI(CORE_FOR_AUDIO_SYSTEM,IPI_AUDIO_UPDATE);
	#endif    
}


void AudioSystem::doUpdate()
{
	#define WITH_TIMING

	#ifdef WITH_TIMING
		uint32_t totalcycles = CTimer::GetClockTicks();
			// GetClockTicks() is in millionths of a second
			// based on the 1Mhz physical counter, so we don't
			// divide it any further.
	#endif
	
	for (AudioStream *p = s_pFirstStream; p; p = p->m_pNextStream)
	{
		if (p->m_numConnections)
		{
			#ifdef WITH_TIMING
				uint32_t cycles =  CTimer::GetClockTicks();
			#endif
			
			p->update();

			// TODO: traverse inputQueueArray and release
			// any input blocks that weren't consumed?

			#ifdef WITH_TIMING
				cycles = (CTimer::GetClockTicks() - cycles);
				p->m_cpuCycles = cycles;
				if (cycles > p->m_cpuCyclesMax)
					p->m_cpuCyclesMax = cycles;
			#endif
		}
	}
	
	#ifdef WITH_TIMING
		totalcycles = (CTimer::GetClockTicks() - totalcycles);
		s_cpuCycles = totalcycles;
		if (totalcycles > s_cpuCyclesMax)
			s_cpuCyclesMax = totalcycles;
	#endif
	
	__disable_irq();
	s_nInUpdate--;
	__enable_irq();    
}

	
