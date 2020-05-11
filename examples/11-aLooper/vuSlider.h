
#ifndef _vuSlider_h
#define _vuSlider_h

#include <audio/Audio.h>
#include <ws/wsWindow.h>
#include <system/midiEvent.h>


class vuSlider : public wsWindow
{
	public:
	
		~vuSlider() {}
		
        vuSlider(
			wsWindow *pParent,
			u16 id,
			s32 xs, s32 ys, s32 xe, s32 ye,
			
			u8 midi_channel,      // midi channel
			u8  midi_cc_num);	  // midi cc control number
        
	protected:
	
		static void staticHandleMidiEvent(void *pThis, midiEvent *event);
		void handleMidiEvent(midiEvent *event);
		
        virtual void onDraw();
        
    private:
    
        u8 m_last_val;
        
};






#endif  // _vuSlider_h
