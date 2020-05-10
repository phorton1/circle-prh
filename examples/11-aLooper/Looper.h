// The Looper Machine
//
// Slightly above the audio layer, this layer implements the audio objects
// that record and playback clips in the looper, but also contains the notion
// of tracks and layers.
//
// It implements the looper state machine, and provides all necessary
// functionality to synchronize clips for playback and to record new ones.
//
// The main limitation of this looper is the fact that it thinks
// in terms of a one pass live loop.  Once some audio is recorded,
// it statys there until the entire Looper is cleared.
//
// It is designed for potiential use in an 6x8 configuration to take
// maximum advantage of the audioInjector Octo sound card, so it
// can record from any of the 6 input channels and playback a raw
// summation of any recorded sounds to any of the 8 output channels,
// though in practice, and initial implementation, it will be limited
// to recording a bunch of stereo pairs and playing them back as a
// stereo pair.
//
// It is a "clip" that determines the number of channels that are
// recorded into a single take. It has the essential contiguous buffer
// memory representing the take. It provides a method to sum the channels
// within it to an abritary number of output channels via a matrix.
//
// In our initial implementation a clip will be a stereo pair,
// output directly to a stereo pair.  The matrix wlll not be
// initially implemented.


#ifndef _loopMachine_h_
#define _loopMachine_h_

#include <audio/Audio.h>


#define USE_INPUT_AMP          1

#define USE_OUTPUT_MIXER       1
    // If 0 the audio inputs will be connected directly to the looper and 
    //    the looper outputs will be connected directly to the audio outputs
    // if 1, there will inputs will be routed through a pair of mixers
    //    before going to the looper, and the audio inputs, and the looper
    //    outputs will be connected to a separate mixer for output, in which
    //    and the looper does not echo the inputs.

#define NO_THRU_LOOPER          1
    // requires USE_OUTPUT_MIXER



    
class loopClip;
class loopTrack;
class loopMachine;

//----------------------------------------------------------
// loopBuffer
//----------------------------------------------------------
// The loopBuffer is a big contiguous piece of memory that
// contains clips. It is implemented as a simple heap with
// the general idea that chunks will be recorded once, and
// typically not re-allocated.  There is no "free" mechanism,
// except to re-initialize the whole buffer.

#define INTEGRAL_BLOCKS_PER_SECOND  ((AUDIO_SAMPLE_RATE + (AUDIO_BLOCK_SAMPLES-1)) / AUDIO_BLOCK_SAMPLES)
    // since there are 344.53125 blocks per second at 44100 and 128, this will be 345
    // it is only used for allocating the initial buffer, NOT for calculating durations
    
#define LOOP_HEAP_BYTES  (60 * 4 * 3 * AUDIO_BLOCK_BYTES * 2 * INTEGRAL_BLOCKS_PER_SECOND)
    // 1440 track seconds, 496,800 blocks == 127,1800,00 bytes == approx 128M
    // 60 seconds of 4 tracks of 3 layers of 128 sample audio
    // blocks (16 bits per sample) in stereo.

class loopBuffer
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


    
//----------------------------------------------------------
// loopClip
//----------------------------------------------------------
// A loop clip is a lightweight object, essentially a buffer,
// representing one "take" within a synchronized track.
//
// The first clip within a track is called the "base clip",
// and is unconstrained in length.  Once the base clip has
// been established, all other clips in the track must be
// an integer multiple of it in length, in terms of audio
// blocks.


#define LOOP_CLIP_CHANNELS   2

#define LOOP_CLIP_STATE_EMPTY       0
#define LOOP_CLIP_STATE_RECORDING   1
#define LOOP_CLIP_STATE_RECORDED    2


class loopClip
{
    public:
        
        loopClip(u16 clip_num, loopTrack* pTrack, u16 num_channels=2);
        ~loopClip()
        {
            m_pLoopTrack = 0;
            m_pLoopBuffer = 0;
        }
        
        u16 getClipNum()            { return m_clip_num; }
        loopTrack *getLoopTrack()   { return m_pLoopTrack; }
        loopBuffer *getLoopBuffer() { return m_pLoopBuffer; }
        
        void setNumChannels(u16 num_channels);
        
        void init()
            // init() MUST must be called on abort of recording
        {
            m_cur_block = 0;
            m_num_blocks = 0;
            m_max_blocks = 0;
            m_buffer = 0;
        }   
        
        void start()
            // start() MUST be called before recording
        {
            m_cur_block = 0;
            m_num_blocks = 0;
            m_max_blocks = m_pLoopBuffer->getFreeBlocks() / m_num_channels;
            m_buffer = m_pLoopBuffer->getBuffer();
        }
        
        // A clip thinks of itself in terms of full stereo (num_channels) blocks
        // This is the only conversion and it is up to the client to know the
        // mapping within the buffer. It is up to the client to make sure that
        // getCurBlock() < getMaxBlocks() before incrementing.
        
        s16 *getBlockBuffer()  { return m_cur_block < m_max_blocks ? &m_buffer[m_cur_block * AUDIO_BLOCK_BYTES * m_num_channels] : 0; }

        u32 getCurBlock()     { return m_cur_block; }         // current song pointer
        u32 getNumBlocks()    { return m_num_blocks; }        // corrrent committed recording 
        u32 getMaxBlocks()    { return m_max_blocks; }        // in terms of stereo "blocks"

        void zeroCurBlock()   { m_cur_block = 0; }            // current play/record pointer
        void incCurBlock()
        {
            m_cur_block++;
            if (m_num_blocks && (m_cur_block == m_num_blocks))
                m_cur_block = 0;
        }

        void commit();
        u16 getClipState()
        {
            if (m_num_blocks) return LOOP_CLIP_STATE_RECORDED;
            if (m_buffer) return LOOP_CLIP_STATE_RECORDING;
            return LOOP_CLIP_STATE_EMPTY;
        }
        
        bool isSelected();
        
    private:
        
        u16        m_clip_num;
        loopTrack  *m_pLoopTrack;
        loopBuffer *m_pLoopBuffer;

        u16  m_num_channels;
        
        u32  m_cur_block;
        u32  m_max_blocks;
        u32  m_num_blocks;
        s16 *m_buffer;
        
};



//-------------------------------------------
// loopTrack
//-------------------------------------------
// a loopTrack consists of a number of
// layers of clips. Only one of them can be "active"
// at a time within the looper.
//
// Like the loopClip, this is mostly a memory object
//
// For the time being, the clips within it are pre-alocated,
// and are recorded in sequential order.


#define LOOPER_NUM_TRACKS     4
#define LOOPER_NUM_LAYERS     3


class loopTrack
{
    public:
        
        loopTrack(u16 track_num, loopMachine *pLooper);
        ~loopTrack();

        u16 getTrackNum()           { return m_track_num; }
        loopMachine *getLooper()    { return m_pLooper; }
        loopBuffer *getLoopBuffer() { return m_pLoopBuffer; }
        
        void init()
        {
            m_selected_clip_num = 0;
            for (int i=0; i<LOOPER_NUM_LAYERS; i++)
                m_clips[i]->init();
        }
        
        void zeroClips()
        {
            for (int i=0; i<LOOPER_NUM_LAYERS; i++)
                m_clips[i]->zeroCurBlock();
        }
        
        u16 getNumClips()   // includes the currently recording clip, if any
        {
            int count = 0;
            for (int i=0; i<LOOPER_NUM_LAYERS; i++)
            {
                if (m_clips[i]->getClipState() > LOOP_CLIP_STATE_EMPTY)
                    count++;
                else
                    break;
            }
            return count;
        }
        u16 getNumRecordedClips()   // only includes recorded clips
        {
            int count = 0;
            for (int i=0; i<LOOPER_NUM_LAYERS; i++)
            {
                if (m_clips[i]->getClipState() >= LOOP_CLIP_STATE_RECORDED)
                    count++;
                else
                    break;
            }
            return count;
        }
        
        
        
        loopClip *getClip(u16 num); // with error checking

        bool isSelected();
        
        void setSelectedClipNum(u16 num)
        {
            if (num < LOOPER_NUM_LAYERS)
                m_selected_clip_num = num;
        }
        u16 getSelectedClipNum()     { return m_selected_clip_num; }
        loopClip *getSelectedClip()  { return m_clips[m_selected_clip_num]; }
        
        
    private:
        
        u16          m_track_num;
        loopMachine *m_pLooper;
        loopBuffer  *m_pLoopBuffer;
        
        loopClip *m_clips[LOOPER_NUM_LAYERS];
        
        u16 m_selected_clip_num;

};



//-----------------------------------------
// looper
//-----------------------------------------
// THIS MUST BE LARGER THAN OR EQUAL TO THE NUMBER
// OF CONNECTIONS THAT ARE ALLOCATED IN audio.cpp

#if 1       // initital stereo in, stereo out looper
    #define LOOPER_MAX_NUM_INPUTS   2
    #define LOOPER_MAX_NUM_OUTPUTS  2
#else   // future 6x8 loooper
    #define LOOPER_MAX_NUM_INPUTS   6
    #define LOOPER_MAX_NUM_OUTPUTS  8
#endif


#define LOOP_STATE_NONE             0
#define LOOP_STATE_RECORDING        1
#define LOOP_STATE_PLAYING          2
#define LOOP_STATE_STOPPED          4


class loopMachine : public AudioStream
{
    public:
        
        loopMachine();
        ~loopMachine();
        
        void init();
        
        virtual const char *getName() 	{ return "looper"; }
        virtual u16   getType()  		{ return AUDIO_DEVICE_OTHER; }
        
        static const char *getLoopStateName(u16 state);
        static const char *getCommandName(u16 name);

        u16 getLoopState()     { return m_state; }
        u16 getPendingState()  { return m_pending_state; }  
        
        loopBuffer *getLoopBuffer()     { return m_pLoopBuffer; }
        loopTrack  *getTrack(u16 num)   { return m_tracks[num]; }

        // All tracks, even empty ones, can be gotten with the API
        // The semantic is that you can only select the 2nd track
        // once the 1st is record.  This is encapsulated in
        // getNumUsedTracks.  Tracks know if they are empty.

        
        u16         getCurTrackNum()        { return m_cur_track_num; }
        loopTrack  *getCurTrack()           { return m_tracks[m_cur_track_num]; }
        
        // Track Selection ...
        //
        // The selected track can be different than the current track.
        // The current track is the one playing or recording at the moment.
        //
        // The user can select one more track than has content, or is
        // being recorded. If a 0th clip is being recorded, and is
        // aborted (so the subsequent track can no longer be reached),
        // the selected track is automatically set back to the current
        // track. 
        
        u16         getNumUsedTracks();
        u16         getSelectedTrackNum()   { return m_selected_track_num; }
        loopTrack  *getSelectedTrack()      { return m_tracks[m_selected_track_num]; }
        
        // state machine API
        
        void command(u16 command, u16 param=0);

    protected:

            void setPendingState(u16 state);

            void selectTrack(u16 num);
            
    private:
        
        volatile u16 m_state;
        u16 m_pending_state;
        
        u16 m_cur_track_num;
        u16 m_selected_track_num;
        
        loopBuffer *m_pLoopBuffer;
        loopTrack *m_tracks[LOOPER_NUM_TRACKS];
      	audio_block_t *inputQueueArray[LOOPER_MAX_NUM_INPUTS];

        virtual void update(void);
        
        loopClip *m_pRecordClip;
            // set at start of recording
        
};


//////////////////////////////////////////////////////
////////// STATIC GLOBAL ACCESSOR ////////////////////
//////////////////////////////////////////////////////

extern loopMachine *pLooper;


//------------------------------------------
// state machine commands
//------------------------------------------

#define LOOP_COMMAND_NONE               0
#define LOOP_COMMAND_CLEAR_ALL          10
#define LOOP_COMMAND_STOP               20
#define LOOP_COMMAND_PLAY               30
#define LOOP_COMMAND_RECORD             40
#define LOOP_COMMAND_SELECT_NEXT_TRACK  50
#define LOOP_COMMAND_SELECT_NEXT_CLIP   60
#define LOOP_COMMAND_STOP_IMMEDIATE     70


// ideas:
// #define LOOP_COMMAND_STOP_IMMEDIATE     21
// #define LOOP_COMMAND_PLAY_IMMEDIATE     31
// #define LOOP_COMMAND_DISABLE_CLIP   5
// #define LOOP_COMMAND_ENABLE_CLIP    5
// #define LOOP_OOMMAND_SELECT_TRACK   10
// #define LOOP_COMMAND_SELECT_LAYER   11

// #define LOOP_COMMAND_SELECT_PREV_TRACK  101
// #define LOOP_COMMAND_SELECT_NEXT_LAYER  102
// #define LOOP_COMMAND_SELECT_PREV_LAYER  103
// #define LOOP_COMMAND_RESTART_TRACK  1000


    
#endif  //!_loopMachine_h_


