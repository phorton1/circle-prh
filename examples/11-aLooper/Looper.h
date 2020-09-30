// The Looper Machine
//
// The classes are broken up into "public" versions, available to the UI,
// and "implementation" versions that are only intended to be called in
// the loopMachine-loopTrack-loopClip hierarchy, while still maintaining
// some protection on the private members of the hiearchy (and a public
// API for use within it).
//
// The UI treats the (public) loopMachine as "readonly", except to
// issue commands() to it, or set the volume/mute of a clip or
// volume of a track.
//
// The machine is driven by the loopMachine::update() method, which
// is called by the audio system for every audio buffer.

#ifndef _loopMachine_h_
#define _loopMachine_h_

#include <audio/Audio.h>
#include "commonDefines.h"
    // LOOPER_NUM_TRACKS and LAYERS, TRACK_STATES, LOOP_COMMANDS, and common CC's

#define WITH_METERS   1

#if 1
    #define LOOPER_LOG(f,...)           pTheLoopMachine->LogUpdate(log_name,f,__VA_ARGS__)
#elif 0
    #define LOOPER_LOG(f,...)           CLogger::Get()->Write(log_name,LogDebug,f,__VA_ARGS__)
#else
    #define LOOPER_LOG(f,...)
#endif


//------------------------------------------------------
// CONSTANTS AND STRUCTS
//------------------------------------------------------

// #define LOOPER_NUM_TRACKS     4
// #define LOOPER_NUM_LAYERS     3
    // in common defines.h

#define LOOPER_NUM_CHANNELS   2
    // the whole thing is stereo

#define CROSSFADE_BLOCKS     30
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
#define LOOPER_NUM_CONTROLS                5

typedef struct              // avoid byte sized structs
{
    u16 value;              // 0..127
    u16 default_value;      // 0..127
    float scale;            // 0..1.0 for my controls; unused for codec
    int32_t multiplier;     // for WITH_INT_VOLUMES
} controlDescriptor_t;

// clip states

#define CLIP_STATE_NONE             0x0000
#define CLIP_STATE_RECORD_IN        0x0001       // The clip is recording the minimum duration fade-in
#define CLIP_STATE_RECORD_MAIN      0x0002       // The clip is recording the main portion
#define CLIP_STATE_RECORD_END       0x0004       // The clip is recording the crossfade out portion
#define CLIP_STATE_RECORDED         0x0010       // The clip contains a full recorded buffer
#define CLIP_STATE_PLAY_MAIN        0x0020       // The clip is playing the main portion
#define CLIP_STATE_PLAY_END         0x0040       // The clip is playing the crossfade out portion


#if 1
#else
    // semi-temporary imposition of "track state" with serial feedback to TE
    // bitwise

    #define TRACK_STATE_EMPTY               0x0000
    #define TRACK_STATE_RECORDING           0x0001
    #define TRACK_STATE_PLAYING             0x0002
    #define TRACK_STATE_STOPPED             0x0004
    #define TRACK_STATE_PENDING_RECORD      0x0008
    #define TRACK_STATE_PENDING_PLAY        0x0010
    #define TRACK_STATE_PENDING_STOP        0x0020

    // loopMachine commands

    #define LOOP_COMMAND_NONE               0x00
    #define LOOP_COMMAND_CLEAR_ALL          0x01
    #define LOOP_COMMAND_STOP_IMMEDIATE     0x02      // stop the looper immediately
    #define LOOP_COMMAND_STOP               0x03      // stop at next cycle point
    #define LOOP_COMMAND_DUB_MODE           0x08      // the dub mode is a shift for the next track button
    #define LOOP_COMMAND_TRACK_BASE         0x10      // the seven possible "track" buttons are 0x10..0x17
        // the above commands can be sent to the loop machine.
        // the following are for internal "pending" command use only
    #define LOOP_COMMAND_RECORD             0x40
    #define LOOP_COMMAND_PLAY               0x50

    // Selected (new) Serial CC numbers

    #define LOOP_CONTROL_BASE_CC   0x65     // rpi recv: for 0..LOOPER_NUM_CONTROLS the value is the volume control (Looper pedal == 0x67)
    #define LOOP_STOP_CMD_STATE_CC 0x26		// rpi send: the value is 0, LOOP_COMMAND_STOP or STOP_IMMEDIATE
    #define LOOP_DUB_STATE_CC      0x25		// rpi send: the value is currently only the DUB state
    #define LOOP_COMMAND_CC        0x24		// rpi recv: the value is the LOOP command
    #define TRACK_STATE_BASE_CC    0x14		// rpi send: value is track state
    #define CLIP_VOL_BASE_CC       0x30		// rpi recv ONLY: for 4 tracks * 3 clips - value is volume 0..127
    #define CLIP_MUTE_BASE_CC      0x40		// rpi send and recv: for 4 tracks * 3 clips - value is mute state

#endif



// An in memory log message

typedef struct logString_type
{
    const char *lname;
    CString *string;
    logString_type *next;
} logString_t;


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
extern CString *getTrackStateName(u16 track_state);
    // in loopTrack.cpp
    // you must delete the CString when done


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

        u32 getSize()        { return m_size; }
        u32 getSizeBlocks()  { return m_size / AUDIO_BLOCK_BYTES; }
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
// a clip is selected in every track, even if the track
// is not selected.

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
        u32 getCrossfadeBlockNum()  { return m_crossfade_start + m_crossfade_offset; }

        bool isMuted()              { return m_mute; }
        void setMute(bool mute)     { m_mute = mute; }

        int getVolume()             { return m_volume * 100.00; }
        void setVolume(int vol)     { m_volume = ((float)vol)/100.00; }
            // midi volumes 0..127 result in 0-1.27 multiplier

    protected:

        void init()
        {
            m_state = 0;
            m_num_blocks = 0;
            m_max_blocks = 0;
            m_play_block = 0;
            m_record_block = 0;
            m_crossfade_start = 0;
            m_crossfade_offset = 0;
            m_mute = false;
            m_volume = 1.0;
        }

        u16  m_track_num;
        u16  m_clip_num;
        u16  m_state;
        u32  m_num_blocks;      // the number of blocks NOT including the crossfade blocks
        u32  m_max_blocks;      // the number of blocks available for recording
        u32  m_play_block;
        u32  m_record_block;
        u32  m_crossfade_start;
        u32  m_crossfade_offset;

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

        // note that all 'post' processing of cross_fade outs,
        // and recording the extra blocks at the end happens
        // in the update() method ... as updateState() is only
        // called on the current and/or selected tracks()

        void update(audio_block_t *in[], audio_block_t *out[]);
        void updateState(u16 cur_command);
        void stopImmediate();


    private:

        loopTrack  *m_pLoopTrack;

        s16 *m_buffer;

        void _startRecording();
        void _startEndingRecording();
        void _finishRecording();

        void _startPlaying();
        void _startFadeOut();
        void _startCrossFadeOut();
        void _endFadeOut();
};



//---------------------------------------------------
// Track
//---------------------------------------------------
// prh - add Volume to track

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
        u16 getNumRunningClips()    { return m_num_running_clips; }
        bool isSelected()           { return m_selected; }

        virtual publicClip *getPublicClip(u16 clip_num) = 0;
        virtual int getTrackState() = 0;

    protected:

        void init()
        {
            m_num_used_clips = 0;
            m_num_recorded_clips = 0;
            m_num_running_clips = 0;
            m_selected = 0;
        }

        u16  m_track_num;
        u16  m_num_used_clips;
        u16  m_num_recorded_clips;
        u16  m_num_running_clips;

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
        void setSelected(bool selected)  { m_selected = selected; }

        void updateState(u16 cur_command);
            // called before update() if there is a command for the
            // current or selected track to handle. The "previous"
            // track is entirely handled in the update() call chain.
        void update(audio_block_t *in[], audio_block_t *out[]);
            // called only once for any track on the
            // previous and/or current tracks.

        void incDecRunning(int inc);
        void incDecNumUsedClips(int inc);
        void incDecNumRecordedClips(int inc);

        void stopImmediate();

    private:

        virtual publicClip *getPublicClip(u16 clip_num) { return (publicClip *) m_clips[clip_num]; }

        loopClip *m_clips[LOOPER_NUM_LAYERS];

        virtual int getTrackState();

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

        virtual void command(u16 command) = 0;

        u16 getRunning()            { return m_running; }
        u16 getPendingCommand()     { return m_pending_command; }
        int getSelectedTrackNum()   { return m_selected_track_num; }

        bool getDubMode()           { return m_dub_mode; }
        void setDubMode(bool dub)   { m_dub_mode = dub; }

        virtual publicTrack *getPublicTrack(u16 num) = 0;

        // controls

        float getMeter(u16 meter, u16 channel);
        u8 getControlValue(u16 control);
        u8 getControlDefault(u16 control);
        void setControl(u16 control, u8 value);

        logString_t *getNextLogString();

    protected:

        // audio system implementation

        virtual void update() = 0;
        virtual const char *getName() 	{ return "looper"; }
        virtual u16   getType()  		{ return AUDIO_DEVICE_OTHER; }

        void init()
        {
            m_running = 0;
            m_pending_command = 0;
            m_selected_track_num = -1;
            m_dub_mode = false;
        }

        // member variables

        AudioCodec *pCodec;
      	audio_block_t *inputQueueArray[LOOPER_NUM_CHANNELS];

        int m_running;
        u16 m_pending_command;
        int m_selected_track_num;
        bool m_dub_mode;

        meter_t m_meter[NUM_METERS];
        controlDescriptor_t m_control[LOOPER_NUM_CONTROLS];

        logString_t *m_pFirstLogString;
        logString_t *m_pLastLogString;

};




class loopMachine : public publicLoopMachine
{
    public:

        loopMachine();
        ~loopMachine();

        loopTrack *getTrack(u16 num)            { return m_tracks[num]; }

        void incDecRunning(int inc);

        void LogUpdate(const char *lname, const char *format, ...);

    private:

        // implementation of public (UI) API

        virtual void command(u16 command);
        virtual publicTrack *getPublicTrack(u16 num)            { return (publicTrack *) m_tracks[num]; }
        virtual void update(void);

        // internal implementation

        void init();
        void updateState();

        // member variables

        u16 m_cur_command;
        int m_cur_track_num;

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
