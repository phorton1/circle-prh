
#include "vuSlider.h"
#include <circle/logger.h>
#include <utils/myUtils.h>

#define log_name "vuslider"



vuSlider::vuSlider(wsWindow *pParent,u16 id, s32 xs, s32 ys, s32 xe, s32 ye,
    u8 horz, u8 num_divs,
			
		const char *in_dev1,
		u16  in_instance1,
		u16  in_channell,

		const char *in_dev2,
		u16  in_instance2,
		u16  in_channel2,
		
		u8 midi_channel,      // midi channel
		u8  midi_cc_num) :	  // midi cc control number
    wsWindow(pParent,id,xs,ys,xe,ye,0)
{
	LOG("vuSlider(%08x) ctor",(u32)this);
	#if 1
		u32 pc;
		u32 sp;
		u32 lr;
		
		asm volatile
		(
			"mov %0,pc\n"
			"mov %1,sp\n"
			"mov %2,lr\n"
			: "=r" (pc),"=r" (sp),"=r" (lr)
		);
		LOG("vuSlider(%08x) pc(%08x) sp(%08x) lr(%08x)",(u32)this,pc,sp,lr);
	#endif
	
    m_horz = horz;
    m_num_divs = num_divs;
	m_last_midi_val = 0;

	for (int i=0; i<2; i++)
    {
		m_pPeak[i] = 0;
		m_pConnection[i] = 0;
	
		m_last_value[i] = m_num_divs;  // unlikely value to cause it to redraw the first time
		m_next_value[i] = 0;         
		m_hold_red[i] = 0;
	}
	
	setAudioDevice(0,in_dev1,in_instance1,in_channell);
	setAudioDevice(1,in_dev2,in_instance2,in_channel2);
	
	if (m_horz ? ((xe-xs+-1) % num_divs) : ((ye-ys-1) % num_divs))
	{
		LOG_WARNING("major dimension-1=%d should be evenly divisible by %d",
			m_horz ? (xe-xs-1) : (ye-ys-1), num_divs);
	}
    
	#if 1
		midiEventHandler::registerMidiHandler(
			this,
			staticHandleMidiEvent,
			-1,		// cable
			midi_channel,	// channel 6, 0 based, as programmed on MPD21
			MIDI_EVENT_CC,	// s8 msg,
			midi_cc_num,	// 0x0D = the middle right knob on MPD218
			-1);			// any values
	#endif

	setBackColor(wsBLACK);
}


void vuSlider::setAudioDevice(
	int 		which,
    const char *name,
    u8         instance,
    u8         channel)
{
    LOG("setAudioDevice(%08x,%d,%s:%d)[%d]",(u32)this,which,name,instance,channel);
    
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
    
    if (m_pConnection[which])
    {
        delete m_pConnection[which];
        m_pConnection[which] = 0;
    }
        
    // create or re-use the peak device
    
    if (!m_pPeak[which])
        m_pPeak[which] = new AudioAnalyzePeak();
    assert(m_pPeak[which]);
        
    // connect the peak device to the output of the sream
    
    if (m_pPeak[which])
    {
        m_pConnection[which] = new AudioConnection(
            *stream, channel, *m_pPeak[which], 0);
        assert(m_pConnection[which]);
    }
	
	pDevice[which] = stream;
}


// virtual
void vuSlider::updateFrame()
{
    // wsWindow::update();
    // wsWindow::updateFrame();
    
    if (m_pPeak[0] && m_pPeak[1])
    {
        float peak0 = m_pPeak[0]->read();
        float peak1 = m_pPeak[1]->read();
        u8 value0 = (peak0 * ((float)m_num_divs) + 0.8);
        u8 value1 = (peak1 * ((float)m_num_divs) + 0.8);
		
        if (value0 != m_next_value[0] ||
			value1 != m_next_value[0])
        {
            // LOG("setting next_value to %d",value);
            m_next_value[0] = value0;
            m_next_value[1] = value1;
            // m_pDC->invalidate(m_rect_abs);
			setBit(m_state,WIN_STATE_DRAW);
		}
    }
}



// virtual
void vuSlider::onDraw()
{
	#if 0
		u32 pc;
		u32 sp;
		u32 lr;
		
		asm volatile
		(
			"mov %0,pc\n"
			"mov %1,sp\n"
			"mov %2,lr\n"
			: "=r" (pc),"=r" (sp),"=r" (lr)
		);
		
		sp += 0x68;
		
		printf("onDraw(%08X) PC(%08x) SP(%08x) LR(%08x)\n",(u32)this,pc,sp,lr);	
		// LOG("onDraw sp=0x%08X",sp);
		//for (int i=0; i<16; i++)
		//{
		//	LOG("stack(%08X)[%d]=0x%08X",(u32)this,i,*(u32 *)(sp+=4));
		//}
	#endif
	
	
	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
    
	// redraw the volume control background
	
	#define BAR_HEIGHT 3
	
	if (m_next_midi_val != m_last_midi_val)
	{
		float fval = (127.0 - m_next_midi_val);
		float height = getClientHeight() - BAR_HEIGHT;
		float pos = fval * height / 127.00;
		s32 ipos = pos;
		
		#if 0
			LOG("drawing(%08x) background %d,%d,%d,%d",
				(u32)this,
				m_rect_client.xs,
				m_rect_client.ys + ipos,
				m_rect_client.xe,
				m_rect_client.ys + ipos + BAR_HEIGHT - 1);
		#endif
		
		m_pDC->fillFrame(
			m_rect_client.xs,
			m_rect_client.ys + ipos,
			m_rect_client.xe,
			m_rect_client.ys + ipos + BAR_HEIGHT - 1,
			wsDARK_BLUE);
	}
	
    // We redraw the bar if the control has been invalidated
    // OR if the values have changed.
	
    u8 num_yellows = (((float) m_num_divs) * 0.20);
	for (int which=0; which<2; which++)
	{
		// if ((m_state & WIN_STATE_INVALID) ||
		// 	m_next_midi_val != m_last_midi_val ||
		// 	m_hold_red[which] ||
		// 	(m_next_value[which] != m_last_value[which]))
		{
			m_last_value[which] = m_next_value[which];
			
			wsRect rect = getClientRect();

			if (m_horz)
			{
				int use_height = (rect.ye - rect.ys - 2) / 2;
				if (which)
				{
					rect.ys += use_height+1; 
				}
				else
				{
					rect.ye = use_height; 
				}
			}
			else
			{
				int use_width = (rect.xe - rect.xs - 2) / 2;

				if (which)
				{
					rect.xs += use_width+1; 
				}
				else
				{
					rect.xe -= use_width+1; 
				}

				// LOG("(%08x) sx,ex=%d,%d",(u32)this,rect.xs,rect.xe);

			}
	
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
				if (m_next_value[which] > i)
				{
					if (i == m_num_divs-1)
					{
						color = wsRED;
						m_hold_red[which] = 16;        // hold for approx 1.5 sec
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
				else if (m_hold_red[which] && (i == m_num_divs-1))
				{
					if (--m_hold_red[which])
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

	{
		float fval = (127.0 - m_last_midi_val);
		float height = getClientHeight() - BAR_HEIGHT;
		float pos = fval * height / 127.00;
		s32 ipos = pos;

		#if 0
			LOG("drawing(%08x) foreground %d,%d,%d,%d",
				(u32)this,
				m_rect_client.xs,
				m_rect_client.ys + ipos,
				m_rect_client.xe,
				m_rect_client.ys + ipos + BAR_HEIGHT - 1);
		#endif
		
		m_pDC->fillFrame(
			m_rect_client.xs,
			m_rect_client.ys + ipos,
			m_rect_client.xe,
			m_rect_client.ys + ipos + BAR_HEIGHT - 1,
			wsWHITE);
	}
	
	m_next_midi_val = m_last_midi_val;
}




void vuSlider::staticHandleMidiEvent(void *pThis, midiEvent *event)
{
	((vuSlider *)pThis)->handleMidiEvent(event);
}


void vuSlider::handleMidiEvent(midiEvent *event)
{
	s8 val = event->getValue2();
	if (m_next_midi_val == m_last_midi_val &&
		val != m_last_midi_val)
	{
		m_last_midi_val = val;
		setBit(m_state,WIN_STATE_DRAW);
		
		if (!strcmp(pDevice[0]->getName(),"amp"))
		{
			((AudioAmplifier *)pDevice[0])->gain(((float)m_last_midi_val)/100.0);
			((AudioAmplifier *)pDevice[1])->gain(((float)m_last_midi_val)/100.0);
		}
	}
}

