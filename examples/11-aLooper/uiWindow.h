
#ifndef _uiWindow_h_
#define _uiWindow_h_

#include <ws/ws.h>


#define ID_WIN_LOOPER    	100

#define NUM_LOOP_BUTTONS 		5


class uiWindow : public wsTopLevelWindow
{
	public:

		uiWindow(wsApplication *pApp, u16 id, s32 xs, s32 ys, s32 xe, s32 ye);

	private:

		virtual void updateFrame();
		virtual u32 handleEvent(wsEvent *event);

		u16 getButtonFunction(u16 num);

		u16 last_running;
		u16 last_num_used_tracks;
		u16 last_button_fxn[NUM_LOOP_BUTTONS];

		#if USE_MIDI_SYSTEM
			wsMidiButton *pLoopButton[NUM_LOOP_BUTTONS];
		#else
			wsButton *pLoopButton[NUM_LOOP_BUTTONS];
		#endif


		CSerialDevice *m_pSerial;

		static void staticSerialReceiveIRQHandler(void *pThis, unsigned char c);
		void serialReceiveIRQHandler(unsigned char c);

		int serial_midi_len;
		unsigned char serial_midi_buf[4];

};

#endif // !_uiWindow_h_
