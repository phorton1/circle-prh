
#ifndef _uiClip_h
#define _uiClip_h

#include <ws/ws.h>
#include <system/midiEvent.h>

#define NUM_LTB_ROWS   3
#define NUM_LTB_COLS   4


class uiClip : public wsWindow
{
	public:

		~uiClip() {}

		uiClip(
				u8 track_num,
				u8 clip_num,
				wsWindow *pParent,
				u16 id,
				s32 xs,
				s32 ys,
				s32 xe,
				s32 ye);

	protected:

		u8 m_track_num;
		u8 m_clip_num;

		u8 m_pressed;

		u16 m_clip_state;	// last clip state from the clip

		u32 m_rec_block;	// last record or play block from the clip
		u32 m_play_block;	// last record or play block from the clip
		u32 m_fade_block;	// last crossfade block number from the clip
		u32 m_num_blocks;	// last number of blocks from the clip
		u32 m_max_blocks;	// last max number of blocks from the clip
		bool m_selected;	// last selected state from the clip

		bool m_flash;
		bool m_last_flash;
		u32  m_flash_time;

		virtual void onDraw();
		virtual void updateFrame();
		virtual void onUpdateTouch(bool touched);

		static void staticHandleMidiEvent(void *pObj, midiEvent *event);
		void handleMidiEvent(midiEvent *event);


};	// uiClip


#endif  // !_uiClip_h
