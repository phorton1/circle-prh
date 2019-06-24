/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2017 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Arduino.h>
#include "AudioStream.h"

#ifdef __circle__
	#define MAX_AUDIO_MEMORY 229376
		// prh - pick a number
	#define USE_AUDIO_UPDATE_TASK
		// otherwise it will be called directly from the interupt
#endif


#if defined(__MKL26Z64__)
  #define MAX_AUDIO_MEMORY 6144
#elif defined(__MK20DX128__)
  #define MAX_AUDIO_MEMORY 12288
#elif defined(__MK20DX256__)
  #define MAX_AUDIO_MEMORY 49152
#elif defined(__MK64FX512__)
  #define MAX_AUDIO_MEMORY 163840
#elif defined(__MK66FX1M0__)
  #define MAX_AUDIO_MEMORY 229376
#endif

#define NUM_MASKS  (((MAX_AUDIO_MEMORY / AUDIO_BLOCK_SAMPLES / 2) + 31) / 32)

audio_block_t * AudioStream::memory_pool;
uint32_t AudioStream::memory_pool_available_mask[NUM_MASKS];
uint16_t AudioStream::memory_pool_first_mask;

uint16_t AudioStream::cpu_cycles_total = 0;
uint16_t AudioStream::cpu_cycles_total_max = 0;
uint16_t AudioStream::memory_used = 0;
uint16_t AudioStream::memory_used_max = 0;



#ifdef __circle__

	#include <circle/logger.h>
	#include <circle/sched/scheduler.h>

	#define log_name  "astream"
	
	volatile u32 AudioStream::update_needed = 0;
	u32 AudioStream::update_overflow = 0;
	
	#ifdef USE_AUDIO_UPDATE_TASK
	
		class AudioUpdateTask : public CTask
		{
		public:

			AudioUpdateTask() {}
			~AudioUpdateTask() {}
			
			void wake()
			{
				if (GetState() == TaskStateBlocked)
				{
					CTask *pTask = this;
					CScheduler::Get()->WakeTask(&pTask);
				}
			}
		
			void Run(void)
			{
				// We start by blocking ourselves.
				// We will be awakened by the update_all() method as needed.
				// 
				// We clear any pending overflow counts, and set update_needed to 1
				// for the first calls, so that we get nice 0's after it.
				
				CTask *pTask = 0;
				CScheduler::Get()->BlockTask(&pTask);
				AudioStream::update_needed = 1;
				AudioStream::update_overflow = 0;
				while (1)
				{
					if (AudioStream::update_needed)
						AudioStream::do_update();
					CScheduler::Get()->BlockTask(&pTask);
				}
			}
		};
	
		AudioUpdateTask *s_pUpdateTask = 0;
		
	#endif	// USE_AUDIO_UPDATE_TASK
	
	
	#ifdef TOPOLOGICAL_SORT_UPDATES

		// static
		u16 AudioStream::num_objects = 0;
		
		#define UPDATE_DEPTH_LIMIT  255
			// for detecting circularity
			// null nodes will be UPDATE_DEPTH_LIMIT+1
			// disconnected nodes will be UPDATE_DEPTH_LIMIT+2
			
		u16 max_update_depth = 0;
		void AudioStream::traverse_update(u16 depth, AudioStream *p)
		{
			if (depth >= UPDATE_DEPTH_LIMIT)
			{
				p->update_depth = depth;				
				LOG_ERROR("update depth limit(%d) reached on %s%i (circular reference)",
					depth, p->dbgName(), p->dbgInstance());
				return;
			}
			if (depth > p->update_depth)
				p->update_depth = depth;
				
			#if 0
				for (u16 i=0; i<depth; i++)
					printf("    ");
				printf("%s%d   depth:%d   max:%d\n",p->dbgName(),p->dbgInstance(),depth,p->update_depth);
			#endif
			
			for (AudioConnection *con=p->destination_list; con; con=con->next_dest)
			{
				traverse_update(depth+1,&con->dst);
			}
		}
	
	#endif	// TOPOLOGICAL_SORT_UPDATES
	
#endif	//__circle__


// Set up the pool of audio data blocks
// placing them all onto the free list
void AudioStream::initialize_memory(audio_block_t *data, unsigned int num)
{
	unsigned int i;
	unsigned int maxnum = MAX_AUDIO_MEMORY / AUDIO_BLOCK_SAMPLES / 2;

	//Serial.println("AudioStream initialize_memory");
	//delay(10);
	if (num > maxnum) num = maxnum;
	__disable_irq();
	memory_pool = data;
	memory_pool_first_mask = 0;
	for (i=0; i < NUM_MASKS; i++) {
		memory_pool_available_mask[i] = 0;
	}
	for (i=0; i < num; i++) {
		memory_pool_available_mask[i >> 5] |= (1 << (i & 0x1F));
	}
	for (i=0; i < num; i++) {
		data[i].memory_pool_index = i;
	}
	__enable_irq();

	// On circle we use this opportunity to sor the
	// audioStream nodes and call begin() on them ..

	#ifdef __circle__
	
		#ifdef TOPOLOGICAL_SORT_UPDATES
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
			
			LOG("topologically sorting audio streams ...",0);
			
			if (!num_objects)
			{
				LOG_ERROR("No AudioStream objects found!!",0);
				return;
			}
			u16 obj_num = 0;
			AudioStream *objs[num_objects];
			for (AudioStream *p = AudioStream::first_update; p; p = p->next_update)
			{
				objs[obj_num++] = p;
				if (!p->num_inputs)
				{
					if (p->destination_list)
					{
						traverse_update(1,p);
					}
					else
					{
						LOG_WARNING("null update node: %s%d",p->dbgName(),p->dbgInstance());
						p->update_depth = UPDATE_DEPTH_LIMIT + 1;
					}
				}
			}
			
			// mark dangling nodes
			
			for (i=0; i<num_objects; i++)
			{
				AudioStream *p = objs[i];
				if (!p->update_depth)
				{
					LOG_WARNING("dangling update node: %s%d",p->dbgName(),p->dbgInstance());
					p->update_depth = UPDATE_DEPTH_LIMIT + 2;
				}
				p->next_update = 0;
			}
			
			// sort em
			
			i = 0;
			while (i < (unsigned int) num_objects-1)
			{
				if (objs[i]->update_depth > objs[i+1]->update_depth)
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
			
			AudioStream::first_update = objs[0];
			AudioStream *prev = objs[0];
			for (i=1; i<num_objects; i++)
			{
				prev->next_update = objs[i];
				prev = objs[i];
			}
			
		#endif	// TOPOLOGICAL_SORT_UPDATES
		
		//-----------------------------------------------------
		// START THE AUDIO STREAMS ...
		//-----------------------------------------------------
		// call begin() on each AudioStream()
		// The first one will likely start an i2s device, clocks, etc.
	
		for (AudioStream *p = AudioStream::first_update; p; p = p->next_update)
		{
			p->begin();
		}

		// Start the audio update task
		
		#ifdef USE_AUDIO_UPDATE_TASK
			s_pUpdateTask = new AudioUpdateTask();
		#endif
		
	#endif	// __circle__
}





// Allocate 1 audio data block.  If successful
// the caller is the only owner of this new block
audio_block_t * AudioStream::allocate(void)
{
	uint32_t n, index, avail;
	uint32_t *p, *end;
	audio_block_t *block;
	uint32_t used;

	p = memory_pool_available_mask;
	end = p + NUM_MASKS;
	__disable_irq();
	index = memory_pool_first_mask;
	p += index;
	while (1) {
		if (p >= end) {
			__enable_irq();
			//Serial.println("alloc:null");
			return NULL;
		}
		avail = *p;
		if (avail) break;
		index++;
		p++;
	}
	n = __builtin_clz(avail);
	avail &= ~(0x80000000 >> n);
	*p = avail;
	if (!avail) index++;
	memory_pool_first_mask = index;
	used = memory_used + 1;
	memory_used = used;
	__enable_irq();
	index = p - memory_pool_available_mask;
	block = memory_pool + ((index << 5) + (31 - n));
	block->ref_count = 1;
	if (used > memory_used_max) memory_used_max = used;
	
	// printf("alloc 0x%08x\n",(uint32_t)block);
	
	//Serial.print("alloc:");
	//Serial.println((uint32_t)block, HEX);
	return block;
}

// Release ownership of a data block.  If no
// other streams have ownership, the block is
// returned to the free pool
void AudioStream::release(audio_block_t *block)
{
	//if (block == NULL) return;
	uint32_t mask = (0x80000000 >> (31 - (block->memory_pool_index & 0x1F)));
	uint32_t index = block->memory_pool_index >> 5;

	__disable_irq();
	if (block->ref_count > 1) {
		block->ref_count--;
	} else {
		// printf("reles: 0x%08x\n",(uint32_t)block);
		//Serial.print("reles:");
		//Serial.println((uint32_t)block, HEX);
		memory_pool_available_mask[index] |= mask;
		if (index < memory_pool_first_mask) memory_pool_first_mask = index;
		memory_used--;
	}
	__enable_irq();
}

// Transmit an audio data block
// to all streams that connect to an output.  The block
// becomes owned by all the recepients, but also is still
// owned by this object.  Normally, a block must be released
// by the caller after it's transmitted.  This allows the
// caller to transmit to same block to more than 1 output,
// and then release it once after all transmit calls.
void AudioStream::transmit(audio_block_t *block, unsigned char index)
{
	for (AudioConnection *c = destination_list; c != NULL; c = c->next_dest) {
		if (c->src_index == index) {
			if (c->dst.inputQueue[c->dest_index] == NULL) {
				c->dst.inputQueue[c->dest_index] = block;
				block->ref_count++;
			}
		}
	}
}


// Receive block from an input.  The block's data
// may be shared with other streams, so it must not be written
audio_block_t * AudioStream::receiveReadOnly(unsigned int index)
{
	audio_block_t *in;

	if (index >= num_inputs) return NULL;
	in = inputQueue[index];
	inputQueue[index] = NULL;
	return in;
}

// Receive block from an input.  The block will not
// be shared, so its contents may be changed.
audio_block_t * AudioStream::receiveWritable(unsigned int index)
{
	audio_block_t *in, *p;

	if (index >= num_inputs) return NULL;
	in = inputQueue[index];
	inputQueue[index] = NULL;
	if (in && in->ref_count > 1) {
		p = allocate();
		if (p) memcpy(p->data, in->data, sizeof(p->data));
		in->ref_count--;
		in = p;
	}
	return in;
}


void AudioConnection::connect(void)
{
	AudioConnection *p;

	if (isConnected) return;
	if (dest_index > dst.num_inputs) return;
	__disable_irq();
	p = src.destination_list;
	if (p == NULL) {
		src.destination_list = this;
	} else {
		while (p->next_dest) {
			if (&p->src == &this->src && &p->dst == &this->dst
				&& p->src_index == this->src_index && p->dest_index == this->dest_index) {
				//Source and destination already connected through another connection, abort
				__enable_irq();
				return;
			}
			p = p->next_dest;
		}
		p->next_dest = this;
	}
	this->next_dest = NULL;
	src.numConnections++;
	src.active = true;

	dst.numConnections++;
	dst.active = true;

	isConnected = true;

	__enable_irq();
}

void AudioConnection::disconnect(void)
{
	AudioConnection *p;

	if (!isConnected) return;
	if (dest_index > dst.num_inputs) return;
	__disable_irq();
	// Remove destination from source list
	p = src.destination_list;
	if (p == NULL) {
		return;
	} else if (p == this) {
		if (p->next_dest) {
			src.destination_list = next_dest;
		} else {
			src.destination_list = NULL;
		}
	} else {
		while (p) {
			if (p == this) {
				if (p->next_dest) {
					p = next_dest;
					break;
				} else {
					p = NULL;
					break;
				}
			}
			p = p->next_dest;
		}
	}
	//Remove possible pending src block from destination
	dst.inputQueue[dest_index] = NULL;

	//Check if the disconnected AudioStream objects should still be active
	src.numConnections--;
	if (src.numConnections == 0) {
		src.active = false;
	}

	dst.numConnections--;
	if (dst.numConnections == 0) {
		dst.active = false;
	}

	isConnected = false;

	__enable_irq();
}


// When an object has taken responsibility for calling update_all()
// at each block interval (approx 2.9ms), this variable is set to
// true.  Objects that are capable of calling update_all(), typically
// input and output based on interrupts, must check this variable in
// their constructors.
bool AudioStream::update_scheduled = false;

bool AudioStream::update_setup(void)
{
	if (update_scheduled) return false;
	#ifndef __circle__
		NVIC_SET_PRIORITY(IRQ_SOFTWARE, 208); // 255 = lowest priority
		NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
	#endif
	
	update_scheduled = true;
	return true;
}

void AudioStream::update_stop(void)
{
	#ifdef __circle__
		// PRH TODO: stop and restart audioUpdateTask, objects in general
	#else
		NVIC_DISABLE_IRQ(IRQ_SOFTWARE);
	#endif
	
	update_scheduled = false;
}

AudioStream * AudioStream::first_update = NULL;



#ifdef __circle__

	// static
	AudioStream *AudioStream::find(const char *name, u8 instance)
	{
		for (AudioStream *p = AudioStream::first_update; p; p = p->next_update)
		{
			if (!strcmp(name,p->dbgName()) && instance==p->dbgInstance())
				return p;
		}
		return NULL;
	}


	void AudioStream::do_update(void)
	{
		uint32_t totalcycles = CTimer::GetClockTicks();
			// GetClockTicks() is in millionths of a second
			// based on the 1Mhz physical counter, so we don't
			// divide it any further.
		
		for (AudioStream *p = AudioStream::first_update; p; p = p->next_update)
		{
			if (p->active)
			{
				uint32_t cycles =  CTimer::GetClockTicks();
				p->update();
				// TODO: traverse inputQueueArray and release
				// any input blocks that weren't consumed?
				cycles = (CTimer::GetClockTicks() - cycles);
				p->cpu_cycles = cycles;
				if (cycles > p->cpu_cycles_max) p->cpu_cycles_max = cycles;
			}
		}
		//digitalWriteFast(2, LOW);
		totalcycles = (CTimer::GetClockTicks() - totalcycles);
		AudioStream::cpu_cycles_total = totalcycles;
		if (totalcycles > AudioStream::cpu_cycles_total_max)
			AudioStream::cpu_cycles_total_max = totalcycles;
			
		__disable_irq();
		update_needed--;
		__enable_irq();
	}


	void AudioStream::update_all(void)
	{
		#ifdef USE_AUDIO_UPDATE_TASK
			if (s_pUpdateTask)
			{
				if (update_needed)
					update_overflow++;
				update_needed++;
				s_pUpdateTask->wake();
			}
		#else
			if (update_needed)
				update_overflow++;
			update_needed++;
			do_update();
		#endif
	}

#else	// !__circle__
	
	void software_isr(void) // AudioStream::update_all()
	{
		AudioStream *p;
	
		ARM_DEMCR |= ARM_DEMCR_TRCENA;
		ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
		uint32_t totalcycles = ARM_DWT_CYCCNT;
		//digitalWriteFast(2, HIGH);
		for (p = AudioStream::first_update; p; p = p->next_update) {
			if (p->active) {
				uint32_t cycles = ARM_DWT_CYCCNT;
				p->update();
				// TODO: traverse inputQueueArray and release
				// any input blocks that weren't consumed?
				cycles = (ARM_DWT_CYCCNT - cycles) >> 4;
				p->cpu_cycles = cycles;
				if (cycles > p->cpu_cycles_max) p->cpu_cycles_max = cycles;
			}
		}
		//digitalWriteFast(2, LOW);
		totalcycles = (ARM_DWT_CYCCNT - totalcycles) >> 4;;
		AudioStream::cpu_cycles_total = totalcycles;
		if (totalcycles > AudioStream::cpu_cycles_total_max)
			AudioStream::cpu_cycles_total_max = totalcycles;
	}

#endif 	// !__circle__
