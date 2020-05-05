// Copyright (C) 2019  Patrick Horton ... have at it!

#include "awsVuMeter.h"
#include <circle/logger.h>
#define log_name "vu_meter"


awsVuMeter::awsVuMeter(wsWindow *pParent,u16 id, s32 xs, s32 ys, s32 xe, s32 ye,
    u8 horz, u8 num_divs) :
    wsWindow(pParent,id,xs,ys,xe,ye,0)
{
    m_horz = horz;
    m_num_divs = num_divs;

    m_pPeak = 0;
    m_pConnection = 0;

    m_running = 1;
    m_last_value = m_num_divs;  // unlikely value to cause it to redraw the first time
    m_next_value = 0;         
    m_hold_red = 0;

    if (m_horz ? ((xe-xs+-1) % num_divs) : ((ye-ys-1) % num_divs))
    {
        LOG_WARNING("major dimension-1=%d should be evenly divisible by %d",
            m_horz ? (xe-xs-1) : (ye-ys-1), num_divs);
    }
    
	setBackColor(wsBLACK);
}


void awsVuMeter::setAudioDevice(
    const char *name,
    u8         instance,
    u8         channel)
{
    LOG("setAudioDevice(%s:%d)[%d]",name,instance,channel);
    
    AudioStream *stream = AudioSystem::find(0,name,instance);
    if (!stream)
    {
        LOG_ERROR("Could not find AudioStream %s:%d",name,instance);
        return;
    }
    if (!stream->getNumOutputs())
    {
        LOG_ERROR("Cannot connect to %s which has no outputs",stream->getName());
        return;
    }
    if (channel >= stream->getNumOutputs())
    {
        LOG_ERROR("Cannot connect to %s[%d] which has only has %d outputs",
            stream->getName(),
            channel,
            stream->getNumOutputs());
        return;
    }
    
    // if there's already a connection we need to unhook it
    // and destroy it
    
    if (m_pConnection)
    {
        delete m_pConnection;
        m_pConnection = 0;
    }
        
    // create or re-use the peak device
    
    if (!m_pPeak)
        m_pPeak = new AudioAnalyzePeak();
    assert(m_pPeak);
        
    // connect the peak device to the output of the sream
    
    if (m_pPeak)
    {
        m_pConnection = new AudioConnection(
            *stream, channel, *m_pPeak, 0);
        assert(m_pConnection);
    }
}


// virtual
void awsVuMeter::update()
{
    wsWindow::update();
    
    if (m_pPeak)
    {
        float peak = m_pPeak->read();
        u8 value = (peak * ((float)m_num_divs) + 0.8);
        if (value != m_next_value)
        {
            // LOG("setting next_value to %d",value);
            m_next_value = value;
            m_pDC->invalidate(m_rect_abs);
        }
    }
    wsWindow::update();
}



// virtual
void awsVuMeter::onDraw()
{
    // LOG("awsVuMeter::onDraw(%d) m_last_value=%d m_next_value=%d",m_pPeak->getInstance(),m_last_value,m_next_value);

	// We don't call the parent wsWindow::onDraw() method
    // because we have no frame (that is all the parent does
    // is draw the frame.
    //
    // wsWindow::onDraw();
    
	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
    
    // We redraw the bar if the control has been invalidated
    // OR if the values have changed.
    
    u8 num_yellows = (((float) m_num_divs) * 0.20);
    if ((m_state & WIN_STATE_INVALID) || m_hold_red || (m_next_value != m_last_value))
    {
        m_last_value = m_next_value;
        wsRect rect = getClientRect();

        #define BLANK_SIZE 1
        s16 scale_size = m_horz ?
            rect.xe - rect.xs + 1 :
            rect.ye - rect.ys + 1;
        s16 div_size = (scale_size+BLANK_SIZE) / m_num_divs;
        s16 start_x = rect.xs;
        s16 start_y = rect.ye - div_size + BLANK_SIZE + 1;
        
        for (u8 i=0; i<m_num_divs; i++)
        {
            wsColor color = wsBLACK;
            if (m_next_value > i)
            {
                if (i == m_num_divs-1)
                {
                    color = wsRED;
                    m_hold_red = 16;        // hold for approx 1.5 sec
                }
                else if (i >= m_num_divs-num_yellows-1)
                {
                    color = wsYELLOW;
                }
                else
                {
                    color = wsGREEN;
                }
            }
            else if (m_hold_red && (i == m_num_divs-1))
            {
                if (--m_hold_red)
                    color = wsRED;
            }
            
            if (m_horz)
            {
                m_pDC->fillFrame(start_x,rect.ys,start_x+div_size-BLANK_SIZE-1,rect.ye,color);
                start_x += div_size;
            }
            else
            {
                m_pDC->fillFrame(rect.xs,start_y,rect.xe,start_y+div_size-BLANK_SIZE-1, color);
                start_y -= div_size;
            }
        }
    }
}

