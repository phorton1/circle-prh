#ifndef __AUDIOPROBEH__
#define __AUDIOPROBEH__

#include "Arduino.h"
#include "AudioStream.h"
#include <circle/screen.h>
#include <ugui/uguicpp.h>


#define PROBE_CHANNELS    2


class AudioProbe : public AudioStream
{
public:
    
	AudioProbe(u16 msInterval=2000, u16 skip=0);
        // skip is to do undersampling
	virtual const char *dbgName()  { return "aprobe"; }        
    
	virtual void update(void);
	void begin(void);
    
    void start();
    void stop();

private:

    friend class AudioProbeTask;
    
    CScreenDevice *m_pScreen;

	u16     m_interval;
    bool    m_started;
    bool    m_running;
    u16     m_height;
    u16     m_width;
    u16     m_offset;
    u16     m_skip;
    
    UG_GUI m_GUI;
	static void setPixel(UG_S16 sPosX, UG_S16 sPosY, UG_COLOR Color);
    void drawDottedLine(u16 x1, u16 y1, u16 x2, u16 y2, UG_COLOR c, u16 on, u16 off );
    
    int16_t *m_buffer[PROBE_CHANNELS];
	audio_block_t *inputQueueArray[PROBE_CHANNELS];
    
    void initBuffers();
    void paint();
    void drawGrid();
    
    
};


#endif  // __AUDIOPROBEH__