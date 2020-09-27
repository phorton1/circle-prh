
#ifndef _vuSlider_h
#define _vuSlider_h

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
			bool horz,
			u16 num_divs,
			s16 meter_num,
			s16 control_num,		// -1 means no controller (no midi registration)
			s16 midi_cable,		    // cable
			s16 midi_channel,        // midi channel
			midiEventType_t midi_type,
			s16 midi_param1,
			s16 midi_param2=-1);	    // midi cc control number

	private:

        virtual void onDraw();
   		virtual void updateFrame();

		static void staticHandleMidiEvent(void *pThis, midiEvent *event);
		void handleMidiEvent(midiEvent *event);

        bool m_horz;
        u16 m_num_divs;
		s16 m_meter_num;
		s16 m_control_num;

        u16 m_last_value[2];		// the value that is showing after onDraw()
        u16 m_next_value[2];        // the value to draw at beginning of onDraw()
        u32 m_hold_red[2];			// a timer for red leds

        u16 m_last_control_value;	// the last drawn control value drawn
		u16 m_next_control_value;	// the next control value to draw

		// optimized drawing pseudo constants

		u16 m_box_height;
		u16 m_box_width;
		u16 m_xoffset;
		u16 m_yoffset;

		float m_usable_bar_area;
		u16 m_bar_height;
		u16 m_bar_width;

		virtual void onUpdateDragMove();

};






#endif  // _vuSlider_h
