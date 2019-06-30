
#ifndef AudioSystem_h
#define AudioSystem_h

#include "AudioTypes.h"

class AudioSystem  // singleton
{
public:

	static bool initialize(u32 num_audio_blocks);
	static void start();
	static void stop();
    static void sortStreams();  
	
	static void AddStream(AudioStream *pStream);
	static AudioStream *find(u32 type, const char *name, s16 instance);
	static u16 getNumStreams()	                { return s_numStreams; }
	static AudioStream *getFirstStream()        { return s_pFirstStream; }

	static void doUpdate();
	static void startUpdate();
	static bool takeUpdateResponsibility();
	static audio_block_t *allocate(void);
	static void release(audio_block_t * block);
	
	static void resetStats();
	static u32 	getCPUCycles()  			{ return s_cpuCycles; }
	static u32 	getCPUCyclesMax()			{ return s_cpuCyclesMax; }
	static u32  getTotalMemoryBlocks()		{ return s_totalBlocks; }
	static u32  getMemoryBlocksUsed()		{ return s_blocksUsed; }
	static u32  getMemoryBlocksUsedMax()	{ return s_blocksUsedMax; }
	
private:
    friend class AudioStream;
	friend class CStatusWindow;
    
    static bool initialize_memory(u32 num_audio_blocks);
	static void traverse_update(u16 depth, AudioStream *p);

	static u16  s_numStreams;
	static u32  s_totalBlocks;
	static u32  s_blocksUsed;
	static u32  s_blocksUsedMax;
	static u32  s_cpuCycles;
    static u32  s_nInUpdate;
	static u32  s_cpuCyclesMax;
	static u32  s_numOverflows;
    static bool s_bUpdateScheduled;
    
    static AudioStream   *s_pFirstStream;
	static AudioStream   *s_pLastStream;
	static audio_block_t *s_pAudioMemory;
    static audio_block_t *s_pFreeBlock;

};

	
#endif	// !AudioSystem_h
