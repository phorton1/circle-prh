// midiButton.cpp

#include "wsMidiButton.h"
#include <circle/logger.h>
#define log_name  "midiButton"

wsMidiButton::wsMidiButton(
        wsWindow *pParent,
        u16 id,
        const char *text,
        s32 xs,
        s32 ys,
        s32 xe,
        s32 ye) :
    wsControl(pParent,id,xs,ys,xe,ye,
        WIN_STYLE_TOUCH |
        WIN_STYLE_3D)
{
	LOG("ctor(%s)",m_text);
	
    // init base class

    m_text = text;
    m_align = ALIGN_CENTER;
    m_fore_color = defaultButtonForeColor;
    m_back_color = defaultButtonReleasedColor;

    // init this class
    
    m_button_state = 0;
    m_pressed = 0;
    m_pressed_back_color = defaultButtonPressedColor;
    m_pressed_fore_color = m_fore_color;
    
    printf("ctor(%s)",text);
}	

		
void wsMidiButton::onDraw()
{
	#if 1
		LOG("onDraw(%s) m_state(0x%08x) m_button_state(0x%04x)",
            m_text,
			m_state,
			m_button_state);
	#endif

	wsColor bc = m_pressed ? m_pressed_back_color : m_back_color;
    wsColor fc = m_pressed ? m_pressed_fore_color : m_fore_color;
    
        // m_back_color starts as defaultButtonReleasedColor
        
	m_pDC->setFont(m_pFont);
	m_pDC->putText(
		bc,fc,
		m_rect_client,
		m_align,
		1,2,
		m_text);
                
	m_pDC->setClip(m_clip_client,m_state & WIN_STATE_INVALID);
    m_pDC->fillFrame(
        m_rect_client.xs,
        m_rect_client.ys,
        m_rect_client.xe,
        m_rect_client.ye,
        bc);

	m_pDC->setClip(m_clip_abs,m_state & WIN_STATE_INVALID);
	m_pDC->draw3DFrame(
		m_rect_abs.xs,
		m_rect_abs.ys,
		m_rect_abs.xe,
		m_rect_abs.ye,
			m_pressed ?
				buttonPressedFrameColors :
				buttonReleasedFrameColors);
	
}




void wsMidiButton::onUpdateTouch(bool touched)
{
	#if 1
		printf("midiButton(%08x)::onUpdateTouch(%d)\n",(u32)this,touched);
	#endif

	m_pressed = touched ? 1 : 0;
	
	// if (touched)
	// {
	// 	m_ltb_state++;
	// 	if (m_ltb_state >= NUM_STATES)
	// 		m_ltb_state = 0;
	// }
	
	setBit(m_state,WIN_STATE_DRAW);
}




// static
void wsMidiButton::staticHandleMidiEvent(void *pObj, midiEvent *event)
{
	((wsMidiButton *)pObj)->handleMidiEvent(event);
}

void wsMidiButton::handleMidiEvent(midiEvent *event)
{
	s8 msg = event->getMsg();
	
	LOG("handleMidiEvent(%s) cable=%d channel=%d msg=%d param1=%d param2=%d",
		m_text,
		event->getCable(),
		event->getChannel(),
		msg,
		event->getValue1(),
		event->getValue2());
	
	m_pressed = msg == 9 ? 1 : 0;
	
	if (msg == 9)	// note on
	{
		//m_ltb_state++;
		//if (m_ltb_state >= NUM_STATES)
		//	m_ltb_state = 0;
	}
	
	setBit(m_state,WIN_STATE_DRAW);
}


