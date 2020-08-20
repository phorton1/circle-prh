// The Looper Machine

#ifndef _loopMachine_h_
#define _loopMachine_h_

#include <audio/Audio.h>

#define DEBUG_UPDATE  1


//------------------------------------------------------
// CONSTANTS AND STRUCTS
//------------------------------------------------------

#define LOOPER_NUM_TRACKS     4
#define LOOPER_NUM_LAYERS     3
#define LOOPER_NUM_CHANNELS   2
    // the whole thing is stereo

#define CROSSFADE_BLOCKS     600
    // The number of buffers (10 == approx 30ms) to continue recording

#define LOOP_HEAP_BYTES  (60 * 4 * 3 * AUDIO_BLOCK_BYTES * 2 * INTEGRAL_BLOCKS_PER_SECOND)
    // 1440 track seconds, 496,800 blocks == 127,1800,00 bytes == approx 128M

#define INTEGRAL_BLOCKS_PER_SECOND  ((AUDIO_SAMPLE_RATE + (AUDIO_BLOCK_SAMPLES-1)) / AUDIO_BLOCK_SAMPLES)
    // 345


// meters

#define METER_INPUT                 0
#define METER_LOOP                  1
#define METER_THRU                  2
#define METER_MIX                   3
#define NUM_METERS                  4

typedef struct
    // the measurements across some number of samples
    // for a single meter, bracketed by calls to getMeter()
{
    s16 min_sample[LOOPER_NUM_CHANNELS];
    s16 max_sample[LOOPER_NUM_CHANNELS];
}   meter_t;


// controls

#define CONTROL_INPUT_GAIN          0
#define CONTROL_THRU_VOLUME         1
#define CONTROL_LOOP_VOLUME         2
#define CONTROL_MIX_VOLUME          3
#define CONTROL_OUTPUT_GAIN         4
#define NUM_CONTROLS                5

typedef struct              // avoid byte sized structs
{
    u16 value;              // 0..127
    u16 default_value;      // 0..127
    float scale;            // 0..1.0 for my controls; unused for codec
    int32_t multiplier;     // for WITH_INT_VOLUMES
} controlDescriptor_t;


// clip states

#define CLIP_STATE_NONE             0x0000
#define CLIP_STATE_RECORD_MAIN      0x0001       // The clip is recording the main portion
#define CLIP_STATE_RECORD_END       0x0002       // The clip is recording the crossfade out portion
#define CLIP_STATE_RECORDED         0x0004       // The clip contains a full recorded buffer
#define CLIP_STATE_PLAY_MAIN        0x0010       // The clip is playing the main portion
#define CLIP_STATE_PLAY_END         0x0020       // The clip is playing the crossfade out portion
#define CLIP_STATE_PENDING_RECORD   0x0100       // The clip will start recording on the next cycle of the base clip
#define CLIP_STATE_PENDING_PLAY     0x0200       // The clip will start playing on the next cycle of the base clip
#define CLIP_STATE_PENDING_STOP     0x0400       // The clip will stop recording or playing on the next cycle of the base clip


// loop machine states

#define LOOP_STATE_NONE           0
    // The looper is freshly initialized, there is no prevous track,
    // and the 0th track and clip is selected on for the next operation.
#define LOOP_STATE_RUNNING        1
    // The looper is running and playing or recording at least one track/clip
#define LOOP_STATE_STOPPING       2
    // The looper has been told to stop, and is in the process of stopping.
    // Nothing else can be done until it is stopped.
#define LOOP_STATE_STOPPED        4
    // The looper contains one or more record clips/tracks but is currently
    // totally halted, and can be started again.


// loopMachine commands

#define LOOP_COMMAND_NONE               0
#define LOOP_COMMAND_STOP_IMMEDIATE     10      // stop the looper immediately
#define LOOP_COMMAND_CLEAR_ALL          20
#define LOOP_COMMAND_STOP               30      // stop at next cycle point
#define LOOP_COMMAND_PLAY               40
#define LOOP_COMMAND_RECORD             50
#define LOOP_COMMAND_SELECT_NEXT_TRACK  60
#define LOOP_COMMAND_SELECT_NEXT_CLIP   70

// forwards

class loopClip;
class loopTrack;

// static externs

extern CString *getClipStateName(u16 clip_state);
    // in loopClip.cpp
    // you must delete the CString when done
extern const char *getLoopStateName(u16 state);
extern const char *getLoopCommandName(u16 name);
    // in loopMachine.cpp


//-----------------------------------------------------
// loopBuffer
//-----------------------------------------------------

class loopBuffer
    // The loopBuffer is a big contiguous piece of memory that
    // contains clips. It is implemented as a simple heap with
    // the general idea that chunks will be recorded once, and
    // typically not re-allocated.  There is no "free" mechanism,
    // except to re-initialize the whole buffer.
{
    public:

        loopBuffer(u32 size=LOOP_HEAP_BYTES);
        ~loopBuffer();

        void init()          { m_top = 0; }

         s16 *getBuffer()     { return &m_buffer[m_top]; }

        u32 getFreeBytes()   { return m_size - m_top; }
        u32 getFreeBlocks()  { return (m_size - m_top) / AUDIO_BLOCK_BYTES; }
        u32 getUsedBytes()   { return m_top; }
        u32 getUsedBlocks()  { return m_top / AUDIO_BLOCK_BYTES; }

        void commitBytes(u32 bytes)    { m_top += bytes; }
        void commitBlocks(u32 blocks)  { m_top += blocks * AUDIO_BLOCK_BYTES; }

    private:

        u32 m_top;
        u32 m_size;
        s16 *m_buffer;
};



//-------------------------------------------------------
// Clip
//-------------------------------------------------------
// a clip is selected in every track


class publicClip
{
    public:

        publicClip(u16 track_num, u16 clip_num)
        {
            m_track_num = track_num;
            m_clip_num = clip_num;
            init();
        }
        virtual ~publicClip()       {}

        u16 getClipNum()            { return m_clip_num; }
        u16 getTrackNum()           { return m_track_num; }
        u16 getClipState()          { return m_state; }
        u32 getNumBlocks()          { return m_num_blocks; }
        u32 getMaxBlocks()          { return m_max_blocks; }
        u32 getPlayBlockNum()       { return m_play_block; }
        u32 getRecordBlockNum()     { return m_record_block; }
        u32 getCrossfadeBlockNum()  { return m_crossfade_block; }

        bool isSelected()           { return m_selected; }
        bool isMuted()              { return m_mute; }
        void setMute(bool mute)     { m_mute = mute; }

        float getVolume()           { return m_volume; }
        void setVolume(float vol)   { m_volume = vol; }

    protected:

        void init()
        {
            m_state = 0;
            m_num_blocks = 0;
            m_max_blocks = 0;
            m_play_block = 0;
            m_record_block = 0;
            m_crossfade_block = 0;
            m_selected = !m_clip_num;
            m_mute = false;
        }

        u16  m_track_num;
        u16  m_clip_num;
        u16  m_state;
        u32  m_num_blocks;      // the number of blocks NOT including the crossfade blocks
        u32  m_max_blocks;      // the number of blocks available for recording
        u32  m_play_block;
        u32  m_record_block;
        u32  m_crossfade_block;

        bool m_selected;
        bool m_mute;

        float m_volume;

};


class loopClip : public publicClip
    // A loop clip is essentially a buffer containing one
    // "take" within a synchronized track.
    //
    // The first clip within a track is called the "base clip",
    // and is unconstrained in length.  Once the base clip has
    // been established, all other clips in the track must be
    // an exact integer multiple of it in "length", in terms of
    // audio blocks.
    //
    // The minimum length of a clip is CROSSFADE_BLOCKS, as
    // every clip starts with a fade in from zero upto the nominal
    // maximum volume (1.0) over the first CROSSFADE_BLOCKS.
    // Setting CROSSFAE_BLOCKS to zero disables crossfades.
    //
    // Every clip records a crossfade-out set of blocks at the end of its creation.
    // The fade-out blocks are NOT included in the nominal "length" of the clip,
    // which is, once again, unbounded on the 0th clip of a track but which is an
    // integral multiplier of the base clip "length" on all other clips in the track.
    //
    // Thus there is a notion of the "main" part of the clip, which
    // does not include the final crossfade-out blocks.
    //
    // A loop clip can be simultaneously in several states at the same time
    // due to the notion of cross fading. For example, it can be recording
    // or playing back the crossfade out, while at the same time it could be
    // playing the beginning of the (next) loop through the same clip.
    //
    // Only one clip in the system, may, at any given time, be recording it's
    // "main" portion, but another clip *may* still be recording it's crossfade
    // out portion.
    //
    // When implemented, muted clips still have their pointers updated and
    // will contuinue to "loop" thru the loopMachine, though they don't
    // actually do the cpu intensive loops through the samples.
{
    public:

        loopClip(u16 clip_num, loopTrack* pTrack);
        virtual ~loopClip();

        void init();
            // init() clears the clip and
            // MUST must be called on abort of recording

        inline s16 *getBlock(u32 block_num)
        {
            return &(m_buffer[block_num * AUDIO_BLOCK_SAMPLES * LOOPER_NUM_CHANNELS]);
        }

        // new implementation

        void setSelected(bool selected)  { m_selected = selected; }

        // void updateState();     // called before update() on tracks and clips
        void update(audio_block_t *in[], audio_block_t *out[]);

        // clip state implementation

        // void incRecBlock();

        void stopImmediate();
        void startPlaying();
        void startRecording();
        void startEndingRecording();
        void finishRecording();

    private:

        loopTrack  *m_pLoopTrack;

        s16 *m_buffer;

};



//---------------------------------------------------
// Track
//---------------------------------------------------

class publicTrack
{
    public:

        publicTrack(u16 track_num)
        {
            m_track_num = track_num;
            init();
        }
        virtual ~publicTrack()      {}

        u16 getTrackNum()           { return m_track_num; }
        u16 getNumUsedClips()       { return m_num_used_clips; }
        u16 getNumRecordedClips()   { return m_num_recorded_clips; }
        u16 getSelectedClipNum()    { return m_selected_clip_num; }

        bool isSelected()           { return m_selected; }

        virtual publicClip *getPublicClip(u16 clip_num) = 0;
        virtual publicClip *getSelectedPublicClip() = 0;

    protected:

        void init()
        {
            m_num_used_clips = 0;
            m_num_recorded_clips = 0;
            m_selected_clip_num = 0;
            m_selected = !m_track_num;
        }

        u16  m_track_num;
        u16  m_num_used_clips;
        u16  m_num_recorded_clips;
        u16  m_selected_clip_num;

        bool m_selected;
};



class loopTrack : public publicTrack
    // A loopTrack consists of a number of clips (also referred to as "layers").
    // There is always a "selected" clip, within the (always existing) selected
    // track, which will be the next clip to be acted upon by commmands.
    //
    // A track, more than a clip, can be in mulitple states simultaneously.
    // It can be playing (a subset) of previously recorded clips, and/or
    // recording a clip, and/or in the process finishing up the crossfade
    // out of a recorded or playing clip.
{
    public:

        loopTrack(u16 track_num);
        virtual ~loopTrack();

        void init();                // called to clear the loopMachine

        loopClip *getClip(u16 num)  { return m_clips[num]; }
        loopClip *getSelectedClip() { return m_clips[m_selected_clip_num]; }
        void setSelected(bool selected)  { m_selected = selected; }
        void setSelectedClipNum(u16 num)
        {
            if (m_selected_clip_num != num)
            {
                m_clips[m_selected_clip_num]->setSelected(false);
                m_selected_clip_num = num;
                m_clips[m_selected_clip_num]->setSelected(true);
            }
        }

        #define UPDATE_STATE_CALLED_AS_PREV         0
        #define UPDATE_STATE_CALLED_AS_CUR          1
        #define UPDATE_STATE_CALLED_AS_SELECTED     2

        void updateState(u16 how_called, u16 loop_state, u16 pending_command);
            // called before update() on tracks and clips
        void update(audio_block_t *in[], audio_block_t *out[]);
            // called only once for any track

        void stopImmediate();

        void incDecNumUsedClips(int inc);
        void incDecNumRecordedClips(int inc);

    private:

        virtual publicClip *getPublicClip(u16 clip_num) { return (publicClip *) m_clips[clip_num]; }
        virtual publicClip *getSelectedPublicClip()     { return (publicClip *) m_clips[m_selected_clip_num]; }

        loopClip *m_clips[LOOPER_NUM_LAYERS];

};



//----------------------------------------------------
// loopMachine
//----------------------------------------------------

class publicLoopMachine : public AudioStream
{
    public:

        publicLoopMachine();
        ~publicLoopMachine();

        // public (UI) API

		virtual bool canDo(u16 command) = 0;
        virtual void command(u16 command) = 0;

        u16 getLoopState()          { return m_loop_state; }
        u16 getPendingCommand()     { return m_pending_command; }
        u16 getSelectedTrackNum()   { return m_selected_track_num; }

        virtual publicTrack *getPublicTrack(u16 num) = 0;
        virtual publicTrack *getSelectedPublicTrack(u16 num) = 0;

        // controls

        float getMeter(u16 meter, u16 channel);
        u8 getControlValue(u16 control);
        u8 getControlDefault(u16 control);
        void setControl(u16 control, u8 value);


    protected:

        // audio system implementation

        virtual void update() = 0;
        virtual const char *getName() 	{ return "looper"; }
        virtual u16   getType()  		{ return AUDIO_DEVICE_OTHER; }

        void init()
        {
            m_loop_state = 0;
            m_pending_command = 0;
            m_selected_track_num = 0;
        }

        // member variables

        AudioCodec *pCodec;
      	audio_block_t *inputQueueArray[LOOPER_NUM_CHANNELS];

        u16 m_loop_state;
        u16 m_pending_command;
        int m_selected_track_num;

        meter_t m_meter[NUM_METERS];
        controlDescriptor_t m_control[NUM_CONTROLS];

};




class loopMachine : public publicLoopMachine
{
    public:

        loopMachine();
        ~loopMachine();

        loopTrack *getTrack(u16 num)            { return m_tracks[num]; }
        loopTrack *getSelectedTrack(u16 num)    { return m_tracks[m_selected_track_num]; }

        void setLoopState(u16 loop_state);
        void setCurTrackNum(int num);
        void setPrevTrackNum(int num);
        void setPendingCommand(u16 command);
        void incDecNumUsedTracks(int inc);


    private:

        // implementation of public (UI) API

		virtual bool canDo(u16 command);
        virtual void command(u16 command);
        virtual publicTrack *getPublicTrack(u16 num)            { return (publicTrack *) m_tracks[num]; }
        virtual publicTrack *getSelectedPublicTrack(u16 num)    { return (publicTrack *) m_tracks[m_selected_track_num]; }
        virtual void update(void);

        // internal implementation

        void init();
        void updateState();
        void selectTrack(u16 num);

        // member variables

        int m_prev_track_num;
        int m_cur_track_num;
        u16 m_num_used_tracks;
        u16 m_num_recorded_tracks;

        loopTrack *m_tracks[LOOPER_NUM_TRACKS];
};


//////////////////////////////////////////////////////
////////// STATIC GLOBAL ACCESSOR ////////////////////
//////////////////////////////////////////////////////

extern loopBuffer  *pTheLoopBuffer;
    // in loopBuffer.cpp

extern loopMachine *pTheLoopMachine;
extern publicLoopMachine *pTheLooper;
    // in audio,cpp

#endif  //!_loopMachine_h_
