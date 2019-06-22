#include "output_probe.h"
#include <circle/sched/scheduler.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>

#define log_name "aprobe"


#define PROBE_BUFFER_BYTES  (PROBE_WIDTH * 2)

// #define PROBE_WIDTH     600
// #define PROBE_CHANNELS    2


class AudioProbeTask : public CTask
{
    public:
        
        AudioProbeTask(AudioProbe *pAudioProbe)
            : m_pAudioProbe(pAudioProbe),
              m_state(0) {}
        ~AudioProbeTask() {}
    
        bool isPaused()  { return m_state; }
        
        void pause()
        {
            LOG("pause",0);
            m_state = 1;
        }
        
        void resume()
        {
            LOG("resume",0);
            m_state = 0;
            if (GetState() == TaskStateBlocked)
            {
                CTask *pTask = this;
                CScheduler::Get()->WakeTask(&pTask);
            }
        }
        
        void Run(void)
        {
            CTask *pTask = 0;
            while (1)
            {
                if (m_state == 1)   // pausing
                {
                    m_state = 2;
                    if (GetState() == TaskStateReady)
                        CScheduler::Get()->BlockTask(&pTask);
                }
                
                if (!m_state)
                {
                    m_pAudioProbe->paint();
                    CScheduler::Get()->MsSleep(m_pAudioProbe->m_interval);
                }
            }
        }
        
    private:
        AudioProbe *m_pAudioProbe;
        u8 m_state;
};

AudioProbeTask *s_pUpdateProbeTask = 0;



//------------------------------------------
// AudioProbe initialization, etc
//------------------------------------------

AudioProbe *s_pAudioProbe = 0;

AudioProbe::AudioProbe(u16 msInterval, u16 skip) :
    AudioStream(PROBE_CHANNELS, inputQueueArray),
    m_interval(msInterval),
    m_skip(skip)
{
    m_started = 0;
    m_running = 0;
    m_height = 0;
    m_width = 0;
    m_offset = 0;
    
    for (u16 i=0; i<PROBE_CHANNELS; i++)
    {
        m_buffer[i] = 0;
    }
    s_pAudioProbe = this;
}    


void AudioProbe::initBuffers()
{
    m_offset = 0;
    for (u16 i=0; i<PROBE_CHANNELS; i++)
    {
        if (!m_buffer[i])
            m_buffer[i] = new s16[m_width];
        assert(m_buffer[i]);
        memset(m_buffer[i],0,m_width * sizeof(s16));
    }
}


void AudioProbe::begin()
{
}

void AudioProbe::stop()
{
    m_running = false;
    if (s_pUpdateProbeTask)
        s_pUpdateProbeTask->pause();
}

void AudioProbe::start()
{
    CDeviceNameService *ns = CDeviceNameService::Get();
    assert(ns);
    m_pScreen = (CScreenDevice *) ns->GetDevice("tty1", FALSE);
    assert(m_pScreen);
    m_width = m_pScreen->GetWidth();
    m_height = m_pScreen->GetHeight();

    initBuffers();
    
    if (!m_started)
    {
        LOG("start(%d,%d)",m_width,m_height);
        m_started = 1;
        int rslt = UG_Init(&m_GUI, setPixel, m_width, m_height);
        if (rslt < 0)
        {
            LOG_ERROR("Could not start GUI: %d",rslt);
        }
        else
        {
            UG_SelectGUI(&m_GUI);
        }
	}
    if (!s_pUpdateProbeTask)
        s_pUpdateProbeTask = new AudioProbeTask(this);
    if (s_pUpdateProbeTask->isPaused())
        s_pUpdateProbeTask->resume();
        
    m_running = true;
}


//-----------------------------------
// drawing primitives
//-----------------------------------

void AudioProbe::setPixel(UG_S16 sPosX, UG_S16 sPosY, UG_COLOR Color)
{
	assert(s_pAudioProbe != 0);
	assert (s_pAudioProbe->m_pScreen != 0);
	s_pAudioProbe->m_pScreen->SetPixel((unsigned) sPosX, (unsigned) sPosY, (TScreenColor) Color);
    CScheduler::Get()->Yield();
}


void AudioProbe::drawDottedLine( u16 x1, u16 y1, u16 x2, u16 y2, UG_COLOR c, u16 on, u16 off )
{
   UG_S16 n, dx, dy, sgndx, sgndy, dxabs, dyabs, x, y, drawx, drawy;

   dx = x2 - x1;
   dy = y2 - y1;
   dxabs = (dx>0)?dx:-dx;
   dyabs = (dy>0)?dy:-dy;
   sgndx = (dx>0)?1:-1;
   sgndy = (dy>0)?1:-1;
   x = dyabs >> 1;
   y = dxabs >> 1;
   drawx = x1;
   drawy = y1;

   setPixel(drawx, drawy,c);
   
   s16 len = 0;
   s16 state = 1;

   if( dxabs >= dyabs )
   {
      for( n=0; n<dxabs; n++ )
      {
         y += dyabs;
         if( y >= dxabs )
         {
            y -= dxabs;
            drawy += sgndy;
         }
         drawx += sgndx;
         
         if (state)
            setPixel(drawx, drawy,c);
         len++;
         if ((state && (len == on)) ||
             (!state && (len == off)))
         {
            len = 0;
            state = !state;
         }
      }
   }
   else
   {
      for( n=0; n<dyabs; n++ )
      {
         x += dxabs;
         if( x >= dyabs )
         {
            x -= dyabs;
            drawx += sgndx;
         }
         drawy += sgndy;
         
         if (state)
             setPixel(drawx, drawy,c);
         len++;
         if ((state && (len == on)) ||
             (!state && (len == off)))
         {
            len = 0;
            state = !state;
         }
         
      }
   }  
}


//--------------------------------------
// update
//--------------------------------------


void AudioProbe::update(void)
{
    // always receive and release input blocks
    // we only re-buffer them if we are running
    
    u16 num = AUDIO_BLOCK_SAMPLES;
    if (m_offset + num >= m_width)
    {
        if (m_offset < m_width)
            num = m_width - m_offset;
        else
            num = 0;
    }

    s16 *ip[PROBE_CHANNELS];
	audio_block_t *in[PROBE_CHANNELS];
	for (u16 i=0; i<PROBE_CHANNELS; i++)
    {
		in[i] = receiveReadOnly(i);
        ip[i] = in[i] ? in[i]->data : 0;
    }
    
    if (m_running)
    {
        u16 i = 0;
        while (i<AUDIO_BLOCK_SAMPLES)
        {
            for (u16 j=0; j<PROBE_CHANNELS; j++)
            {
                s16 v = 0;
                if (ip[j])
                {
                    v = *ip[j];
                    ip[j] += m_skip + 1;
                }
                m_buffer[j][m_offset] = v;
            }
            m_offset++;
            if (m_offset >= m_width)
                m_offset = 0;
                
            i += m_skip + 1;
        }
    }
        
	for (u16 i=0; i<PROBE_CHANNELS; i++)
    {
        if (in[i])
            release(in[i]);
    }
}




//--------------------------------------
// paint
//--------------------------------------



void AudioProbe::drawGrid()
{
    #define NUM_HORZ_DIVS  6
    #define NUM_VERT_DIVS  18
    
    u16 channel_height = m_height/PROBE_CHANNELS;
    u16 tick_height = channel_height / NUM_HORZ_DIVS;
    u16 width_per_tick = m_width / NUM_VERT_DIVS;
        
    // LOG("height(%d) width(%d) channel(%d) tick(%d)=",m_height,m_width,channel_height,tick_height);
    
    for (u16 channel=0; channel<PROBE_CHANNELS; channel++)
    {
        u16 start_y = channel * channel_height;
        u16 end_y = start_y + channel_height - 1;
        u16 zero_y = start_y + (end_y - start_y) / 2;
        
        UG_DrawLine(0, zero_y, m_width-1, zero_y, C_DARK_BLUE);
        
        for (u16 tick=1; tick < NUM_HORZ_DIVS; tick++)
        {
            if (tick != NUM_HORZ_DIVS/2)
            {
                u16 tick_y = start_y + tick * tick_height;
                drawDottedLine(0, tick_y, m_width-1, tick_y, C_DARK_BLUE, 3, 7);
            }
        }
        for (u16 tick=1; tick < NUM_VERT_DIVS-1; tick++)
        {
            u16 tick_x = tick * width_per_tick;
            drawDottedLine(tick_x, 0, tick_x, m_height-1, C_DARK_BLUE, 3, 7);
        }
        
        if (channel < PROBE_CHANNELS-1)
            UG_DrawLine(0, end_y, m_width-1, end_y, C_DARK_GRAY);
    }

    UG_DrawFrame( 0, 0, m_width-1, m_height-1, C_DARK_GRAY);
}


void AudioProbe::paint()
{
    UG_FillScreen( C_BLACK );
    drawGrid();
    
    s16 channel_height = m_height/PROBE_CHANNELS;
    s16 div = 32768 / (channel_height / 2);

    // LOG("paint channel_height(%i) div(%i)",channel_height,div);
    
    for (u16 i=0; i<PROBE_CHANNELS; i++)
    {
        s16 *ip = m_buffer[i];
        s16 zero_y = i*channel_height + channel_height/2;
        for (u16 j=0; j<m_width; j++)
        {
            s16 in = *ip++;
            s16 y = (in / div);
            setPixel(j,zero_y  + y, C_YELLOW);
        }
    }
}
