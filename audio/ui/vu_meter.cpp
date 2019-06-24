// Copyright (C) 2019  Patrick Horton ... have at it!

#include "vu_meter.h"
#include <circle/logger.h>
#define log_name "vu_meter"


CVuMeter::CVuMeter(CWindow *win, u8 instance, u8 id, s16 x, s16 y, s16 xe, s16 ye,
    u8 horz, u8 num_divs) :
    CTextbox(win, id, x, y, xe, ye, "")
{
    m_last_value = 0;
    m_horz = horz;
    m_num_divs = num_divs;
    m_hold_red = 0;
    m_pAudioObj = (AudioAnalyzePeak *) AudioStream::find("peak",instance);
    assert(m_pAudioObj);
    if (m_pAudioObj)
        m_pAudioObj->registerUIHandler(33,uiCallbackStub,this);
        // call us back every 33 audio buffers to update the meter,
        // or about 10 times a second

    LOG("new VuMeter(%s:%d)",m_pAudioObj->dbgName(),m_pAudioObj->dbgInstance());
    if (m_horz ? ((xe-x+2) % num_divs) : ((ye-y+2) % num_divs))
    {
        LOG_WARNING("major dimension+1=%d should be evenly divisible by %d",
            m_horz ? (xe-x+1) : (ye-y+1), num_divs);
    }

    SetBackColor(C_BLACK);
    SetForeColor(C_RED);
}

void CVuMeter::uiCallback()
{
    if (!m_pAudioObj)
        return;
    float peak = m_pAudioObj->read();
    // return;

    UG_OBJECT* obj=_UG_SearchObject(m_window,OBJ_TYPE_TEXTBOX,m_id);
    assert(obj);
    
    u8 value = (peak * ((float)m_num_divs) + 0.8);
    u8 num_yellows = (((float) m_num_divs) * 0.20);
    // printf("peak=%0.4f value=%d red=%d\n",peak,value,m_hold_red);
    
    if (m_hold_red || (value != m_last_value))
    {
        m_last_value = value;
        UG_AREA *pArea = &obj->a_abs;
        
        #define BLANK_SIZE 1
        s16 scale_size = m_horz ?
            pArea->xe - pArea->xs + 1 :
            pArea->ye - pArea->ys + 1;
        s16 div_size = (scale_size+BLANK_SIZE) / m_num_divs;
        s16 start_x = pArea->xs;
        s16 start_y = pArea->ye - div_size + BLANK_SIZE + 1;
        
        for (u8 i=0; i<m_num_divs; i++)
        {
            u16 color = C_BLACK;
            if (value > i)
            {
                if (i == m_num_divs-1)
                {
                    color = C_RED;
                    m_hold_red = 16;        // hold for approx 1.5 sec
                }
                else if (i >= m_num_divs-num_yellows-1)
                {
                    color = C_YELLOW;
                }
                else
                {
                    color = C_GREEN;
                }
            }
            else if (m_hold_red && (i == m_num_divs-1))
            {
                if (--m_hold_red)
                    color = C_RED;
            }
            
            if (m_horz)
            {
                UG_FillFrame(start_x,pArea->ys,start_x+div_size-BLANK_SIZE-1,pArea->ye,color);
                start_x += div_size;
            }
            else
            {
                UG_FillFrame(pArea->xs,start_y,pArea->xe,start_y+div_size-BLANK_SIZE-1, color);
                start_y -= div_size;
            }
        }
    }
}

