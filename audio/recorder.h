#ifndef __AUDIORECORDERH__
#define __AUDIORECORDERH__

#include "Arduino.h"
#include "AudioStream.h"

// this device just samples the stream into a buffer
// the buffer is public for the UI to display it

#define RECORD_CHANNELS         4
#define RECORD_SECONDS          30
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

	virtual const char *getName() 	{ return "recorder"; }
	virtual u16   getType()  		{ return AUDIO_DEVICE_OTHER; }
        
    u16  getRecordMask()            { return m_record_mask; }
    void setRecordMask(u16 mask)    { m_record_mask = mask; }
    u16  getPlayMask()              { return m_play_mask; }
    void setPlayMask(u16 mask)      { m_play_mask = mask; }
    
    u32 getLength()                 { return m_num_blocks; }
    u32 getLocation()               { return m_cur_block; }
    
	void start(void);
    bool isRunning()                { return m_running; }
    
    void clearRecording();   // erase existing recording
    void startRecording();   // start recording or playing (if there's blocks)
    void stopRecording();    // stop recording or playing

    int16_t *getBuffer(int channel)            { return m_buffer[channel]; }
    
private:

    u16     m_record_mask;
    u16     m_play_mask;

    bool    m_running;
    u32     m_num_blocks;
    u32     m_cur_block;
    
	audio_block_t *inputQueueArray[RECORD_CHANNELS];
    int16_t *m_buffer[RECORD_CHANNELS];

    void update(void);
    
};


#endif  // !__AUDIORECORDERH__