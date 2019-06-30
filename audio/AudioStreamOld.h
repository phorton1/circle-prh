// Teensyduino Core Library
// http://www.pjrc.com/teensy/
// Copyright (c) 2017 PJRC.COM, LLC.
// circle only
 
#ifndef AudioStream_h
#define AudioStream_h

#include "Arduino.h"

#define AUDIO_BLOCK_SAMPLES  		128	//  32	64  128
#define AUDIO_SAMPLE_RATE_EXACT 	44117.64706
#define AUDIO_BLOCK_BYTES  			(AUDIO_BLOCK_SAMPLES * sizeof(s16))

#define TOPOLOGICAL_SORT_UPDATES

#define SET_AUDIO_INSTANCE() \
	instance_num = next_instance_num++;
#define DEFINE_AUDIO_INSTANCE(class_name) \
	u8 class_name::next_instance_num = 0;

#define AUDIO_SAMPLE_RATE AUDIO_SAMPLE_RATE_EXACT

class AudioStream;
// class AudioConnection;


typedef struct audio_block_struct
{
	uint8_t  ref_count;
	uint8_t  reserved1;
	uint16_t memory_pool_index;
	int16_t  data[AUDIO_BLOCK_SAMPLES];
} audio_block_t;



class AudioConnection
{
public:
	friend class AudioStream;
	
	AudioConnection(
			AudioStream &source,
			AudioStream &destination) :
		src(source),
		dst(destination),
		src_index(0),
		dest_index(0),
		next_dest(NULL)
	{
		isConnected = false;
		connect();
	}

	AudioConnection(
			AudioStream &source,
			unsigned char sourceOutput,
			AudioStream &destination,
			unsigned char destinationInput) :
		src(source),
		dst(destination),
		src_index(sourceOutput),
		dest_index(destinationInput),
		next_dest(NULL)
	{
		isConnected = false;
		connect();
	}

	~AudioConnection()
	{
		disconnect();
	}
	
	void disconnect(void);
	void connect(void);
	
protected:
	
	AudioStream &src;
	AudioStream &dst;
	unsigned char src_index;
	unsigned char dest_index;
	AudioConnection *next_dest;
	bool isConnected;
};



#define AudioMemory(num) ({ \
	static DMAMEM audio_block_t data[num]; \
	AudioStream::initialize_memory(data, num); \
})

#define CYCLE_COUNTER_APPROX_PERCENT(n) \
	(((n) + (F_CPU / 32 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100)) / \
	(F_CPU / 16 / AUDIO_SAMPLE_RATE * AUDIO_BLOCK_SAMPLES / 100))

#define AudioProcessorUsage() (CYCLE_COUNTER_APPROX_PERCENT(AudioStream::cpu_cycles_total))
#define AudioProcessorUsageMax() (CYCLE_COUNTER_APPROX_PERCENT(AudioStream::cpu_cycles_total_max))
#define AudioProcessorUsageMaxReset() (AudioStream::cpu_cycles_total_max = AudioStream::cpu_cycles_total)
#define AudioMemoryUsage() (AudioStream::memory_used)
#define AudioMemoryUsageMax() (AudioStream::memory_used_max)
#define AudioMemoryUsageMaxReset() (AudioStream::memory_used_max = AudioStream::memory_used)



class AudioStream
{
public:
	friend class AudioUpdateTask;
	
	AudioStream(unsigned char ninput, audio_block_t **iqueue);

	virtual const char *dbgName() = 0;
	virtual u8 dbgInstance()    { return 0; }
	virtual u8 getNumOutputs()	{ return 0; }
	
	virtual void begin() {};
	virtual void update(void) = 0;

	bool isActive(void) { return active; }
	void transmit(audio_block_t *block, unsigned char index = 0);
	audio_block_t * receiveReadOnly(unsigned int index = 0);
	audio_block_t * receiveWritable(unsigned int index = 0);

	static void initialize_memory(audio_block_t *data, unsigned int num);

	static void do_update(void);
	static void update_all(void);
	static void update_stop(void);
	static bool update_setup(void);
	static AudioStream *find(const char *name, u8 instance);
	
	static audio_block_t *allocate(void);
	static void release(audio_block_t * block);

	void resetStats();
	static void resetAllStats();
	int processorUsage(void) { return CYCLE_COUNTER_APPROX_PERCENT(cpu_cycles); }
	int processorUsageMax(void) { return CYCLE_COUNTER_APPROX_PERCENT(cpu_cycles_max); }
	void processorUsageMaxReset(void) { cpu_cycles_max = cpu_cycles; }

	
	bool 	 active;
	uint8_t  num_inputs;
	uint8_t  numConnections;
	uint16_t cpu_cycles;
	uint16_t cpu_cycles_max;
	uint16_t last_cpu_cycles;
	uint16_t last_cpu_cycles_max;

	audio_block_t 	**inputQueue;
	AudioConnection *destination_list;
	AudioStream 	*next_update; 
	
	static AudioStream *first_update;

	static bool 	update_scheduled;
	static volatile u32 update_needed;
	static uint16_t cpu_cycles_total;
	static uint16_t cpu_cycles_total_max;
	static uint16_t memory_used;
	static uint16_t memory_used_max;
	static u32 		update_overflow;
	
	static audio_block_t *memory_pool;
	static uint32_t memory_pool_available_mask[];
	static uint16_t memory_pool_first_mask;
	
	#ifdef TOPOLOGICAL_SORT_UPDATES
		static u16 num_objects;
		u16 update_depth;
		static void traverse_update(u16 depth, AudioStream *p);
	#endif
	
};


#endif	// !AudioStream_h
