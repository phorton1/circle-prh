
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
			u8 horz, u8 num_divs,
			
			const char *in_dev1,
			u16  in_instance1,
			u16  in_channel1,
			
			const char *in_dev2,
			u16  in_instance2,
			u16  in_channel2,
			
			u8 midi_channel,      // midi channel
			u8  midi_cc_num);	  // midi cc control number
        
	private:

		void setAudioDevice(
			int 		which,
			const char *name,
			u8         instance,
			u8         channel);

        virtual void onDraw();
   		virtual void updateFrame();
			
		static void staticHandleMidiEvent(void *pThis, midiEvent *event);
		void handleMidiEvent(midiEvent *event);
    
        AudioAnalyzePeak *m_pPeak[2];
        AudioConnection  *m_pConnection[2];
        
        u8 m_horz;
        u8 m_num_divs;

        u8 m_last_value[2];
        u8 m_next_value[2];
        
        u16 m_hold_red[2];
	
        u8 m_last_midi_val;
		u8 m_next_midi_val;
		
		
		AudioStream *pDevice[2];
        
};






#endif  // _vuSlider_h
