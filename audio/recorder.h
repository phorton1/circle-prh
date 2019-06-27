#ifndef __AUDIORECORDERH__
#define __AUDIORECORDERH__

#include "Arduino.h"
#include "AudioStream.h"

// this device just samples the stream into a buffer
// the buffer is public for the UI to display it

#define RECORD_CHANNELS         4
#define RECORD_SECONDS          1
#define RECORD_SAMPLE_RATE      44100
    // we always fill an integral number of AUDIO_BLOCK_SAMPLES
    // the seconds is approximate
#define RECORD_BUFFER_BLOCKS    ((RECORD_SECONDS * RECORD_SAMPLE_RATE) / AUDIO_BLOCK_SAMPLES)
#define RECORD_BUFFER_SAMPLES   (RECORD_BUFFER_BLOCKS * AUDIO_BLOCK_SAMPLES)
#define RECORD_BUFFER_BYTES     (RECORD_BUFFER_SAMPLES * sizeof(s16))


class AudioRecorder : public AudioStream
{
public:
    
	AudioRecorder();
        // skip does undersampling

	void begin(void);
	virtual void update(void);
    virtual const char *dbgName()   { return "recorder"; }
    virtual u8 getNumOutputs()	    { return RECORD_CHANNELS; }

    u16  getRecordMask()            { return m_record_mask; }
    void setRecordMask(u16 mask)    { m_record_mask = mask; }
    u16  getPlayMask()              { return m_play_mask; }
    void setPlayMask(u16 mask)      { m_play_mask = mask; }
    
    u32 getLength()                 { return m_num_blocks; }
    u32 getLocation()               { return m_cur_block; }
    
    bool isRunning()                { return m_running; }
    
    void clear();   // erase existing recording
    void start();   // start recording or playing (if there's blocks)
    void stop();    // stop recording or playing

    int16_t *getBuffer(int channel)            { return m_buffer[channel]; }
    
private:

    u16     m_record_mask;
    u16     m_play_mask;

    bool    m_running;
    u32     m_num_blocks;
    u32     m_cur_block;
    
	audio_block_t *inputQueueArray[RECORD_CHANNELS];
    static int16_t m_buffer[RECORD_CHANNELS][RECORD_BUFFER_SAMPLES];
   
};


#endif  // !__AUDIORECORDERH__