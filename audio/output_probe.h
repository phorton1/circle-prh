#ifndef __AUDIOPROBEH__
#define __AUDIOPROBEH__

#include "Arduino.h"
#include "AudioStream.h"

// this device just samples the stream into a buffer
// the buffer is public for the UI to display it

#define PROBE_CHANNELS    4
#define PROBE_SAMPLES     44100


class AudioProbe : public AudioStream
{
public:
    
	AudioProbe(u16 skip=0);
        // skip does undersampling

	void begin(void);
	virtual void update(void);
    
    virtual const char *dbgName()  { return "aprobe"; }        
    static AudioProbe *Get()       { return s_pAudioProbe; }
    bool isRunning()               { return m_running; }
    
    void start();
    void stop();

    static int16_t m_buffer[PROBE_CHANNELS][PROBE_SAMPLES];
    
private:

    void    initBuffers();
    
    static  AudioProbe *s_pAudioProbe;
    bool    m_running;
    u32     m_offset;
    u16     m_skip;
    
	audio_block_t *inputQueueArray[PROBE_CHANNELS];
   
};


#endif  // __AUDIOPROBEH__