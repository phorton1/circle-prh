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


#ifndef _Looper_h_
#define _Looper_h_

#include <audio/Audio.h>

class LoopClip;
class LoopTrack;
class Looper;

//----------------------------------------------------------
// LoopBuffer
//----------------------------------------------------------
// The LoopBuffer is a big contiguous piece of memory that
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

class LoopBuffer
{
    public:
        
        LoopBuffer(u32 size=LOOP_HEAP_BYTES);
        ~LoopBuffer();
        
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
// LoopClip
//----------------------------------------------------------
// A loop clip is a lightweight object, essentially a buffer,
// representing one "take" within a synchronized track.
//
// The first clip within a track is called the "base clip",
// and is unconstrained in length.  Once the base clip has
// been established, all other clips in the track must be
// an integer multiple of it in length, in terms of audio
// blocks.
//
// 


#define LOOP_CLIP_CHANNELS   2


class LoopClip
{
    public:
        
        LoopClip(u16 clip_num, LoopTrack* pTrack, u16 num_channels=2);

        ~LoopClip()
        {
            m_pLoopTrack = 0;
            m_pLoopBuffer = 0;
        }
        
        u16 getClipNum()            { return m_clip_num; }
        LoopTrack *getLoopTrack()   { return m_pLoopTrack; }
        LoopBuffer *getLoopBuffer() { return m_pLoopBuffer; }
        
        void setNumChannels(u16 num_channels);
        
        void init()
        {
            m_cur_block = 0;
            m_num_blocks = 0;
            m_max_blocks = 0;
            m_buffer = 0;
        }   
        
        void start()
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
        //
        // note also that start() must be called: assert(m_buffer);
        
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
        
    private:
        
        u16        m_clip_num;
        LoopTrack  *m_pLoopTrack;
        LoopBuffer *m_pLoopBuffer;

        u16  m_num_channels;
        u16  m_state;
        
        u32  m_cur_block;
        u32  m_max_blocks;
        u32  m_num_blocks;
        s16 *m_buffer;
        
};



//-------------------------------------------
// LoopTrack
//-------------------------------------------
// a LoopTrack consists of a number of
// layers of clips. Only one of them can be "active"
// at a time within the Looper.
//
// Like the LoopClip, this is mostly a memory object
//
// For the time being, the clips within it are pre-alocated,
// and are recorded in sequential order.


#define LOOPER_NUM_TRACKS     4
#define LOOPER_NUM_LAYERS     3


class LoopTrack
{
    public:
        
        LoopTrack(u16 track_num, Looper *pLooper);
        ~LoopTrack();

        u16 getTrackNum()           { return m_track_num; }
        Looper *getLooper()         { return m_pLooper; }
        LoopBuffer *getLoopBuffer() { return m_pLoopBuffer; }
        
        void init()
        {
            m_num_clips = 0;            // number already recorded
            
                // recording always takes place on the next available clip
                // this is connoted by the getClip(getNumClips()), which
                // MAY OVERFLOW ... you have to check before recording!
            
            for (int i=0; i<LOOPER_NUM_LAYERS; i++)
                m_clips[i]->init();
        }
        
        void zeroClips()
        {
            for (int i=0; i<LOOPER_NUM_LAYERS; i++)
                m_clips[i]->zeroCurBlock();
        }
        
        u16 getNumClips()           { return m_num_clips; }
        LoopClip *getClip(u16 num); // with error checking
        void commit_recording();

        bool isSelected()  { return m_track_num & 1 ? false : true; }
        
    private:
        
        u16         m_track_num;
        Looper     *m_pLooper;
        LoopBuffer *m_pLoopBuffer;
        
        u16 m_num_clips;                // number recorded, clips
        LoopClip *m_clips[LOOPER_NUM_LAYERS];

};



//-----------------------------------------
// Looper
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


class Looper : public AudioStream
{
    public:
        
        Looper();
        ~Looper();
        
        void init();
        
        virtual const char *getName() 	{ return "looper"; }
        virtual u16   getType()  		{ return AUDIO_DEVICE_OTHER; }
        
        const char *getLooperStateName(u16 state);
        u16   getLooperState()          { return m_state; }
        
        LoopBuffer *getLoopBuffer()     { return m_pLoopBuffer; }
        LoopTrack  *getTrack(u16 num)   { return m_tracks[num]; }

        // All tracks, even empty ones, can be gotten with the API
        // The semantic is that you can only select the 2nd track
        // once the 1st is record.  This is encapsulated in
        // getNumUsedTracks.  Tracks know if they are empty.

        u16 getNumUsedTracks()          { return m_num_used_tracks; }
        
        LoopTrack  *getCurTrack()       { return m_tracks[m_cur_track_num]; }
        
        // Selection not implemented yet
        
        LoopTrack  *getSelectedTrack()      { return m_tracks[m_selected_track_num]; }
        u16         getCurTrackNum()        { return m_cur_track_num; }
        u16         getSelectedTrackNum()   { return m_selected_track_num; }
        
        // state machine API
        
        void command(u16 command, u16 param=0);

    protected:

            void setLooperState(u16 state);
            void setPendingState(u16 state);

    private:
        
        volatile u16 m_state;
        u16 m_pending_state;
        
        u16 m_num_used_tracks;        // number with content
        u16 m_cur_track_num;
        u16 m_selected_track_num;
        
        LoopBuffer *m_pLoopBuffer;
        LoopTrack *m_tracks[LOOPER_NUM_TRACKS];
      	audio_block_t *inputQueueArray[LOOPER_MAX_NUM_INPUTS];

        virtual void update(void);
        
};


//////////////////////////////////////////////////////
////////// STATIC GLOBAL ACCESSOR ////////////////////
//////////////////////////////////////////////////////

extern Looper *pLooper;


//------------------------------------------
// state machine commands
//------------------------------------------

#define LOOP_COMMAND_NONE           0
#define LOOP_COMMAND_CLEAR_ALL      1
#define LOOP_COMMAND_STOP           2
#define LOOP_COMMAND_PLAY           3
#define LOOP_COMMAND_RECORD         4

#define LOOP_COMMAND_DISABLE_CLIP   5
#define LOOP_COMMAND_ENABLE_CLIP   5

#define LOOP_OOMMAND_SELECT_TRACK   10
#define LOOP_COMMAND_SELECT_LAYER   11

#define LOOP_COMMAND_SELECT_NEXT_TRACK  100
#define LOOP_COMMAND_SELECT_PREV_TRACK  101
#define LOOP_COMMAND_SELECT_NEXT_LAYER  102
#define LOOP_COMMAND_SELECT_PREV_LAYER  103

#define LOOP_COMMAND_RESTART_TRACK  1000


    
#endif  //!_Looper_h_


