
#include "Looper.h"
#include <circle/logger.h>
#include <circle/synchronize.h>

#define log_name "lmachine"

#define WITH_VOLUMES       1
#define WITH_INT_VOLUMES   0

#if WITH_VOLUMES
    #if WITH_INT_VOLUMES
    
        #include <audio/utility/dspinst.h>
        
        #define MULTI_UNITYGAIN 65536
    
        static void applyGain(int16_t *data, int32_t mult)
        {
            // uint32_t tmp32 = *data; // read 2 samples from *data
            int32_t val1 = signed_multiply_32x16b(mult, *data);
            val1 = signed_saturate_rshift(val1, 16, 0);
            *data = val1;
        }
    #endif
#endif


extern int g_sUSBHIDDeviceDelay;    // 0
extern int g_sUSBMIDIDeviceDelay;   // 10
    // various values to try to optimize USB usage
void setUSBOpts()
    // done this way for recompiles over and over
{
    g_sUSBHIDDeviceDelay    = 0;    // 180;
    g_sUSBMIDIDeviceDelay   = 10;   // 60;
}


// static
stereoSample_t peak[NUM_METERS];



// The default input gain for the cs42448 of 0db
// 0.723 = (128.0 / (128.0 + 1.0 + 48.0))
// 91.84 =  0.723 * 127
// My volumes are 100 based to allow for gain
// The output gain is limited to 1.0

u16 control_default[NUM_CONTROLS] = {
    94,      // codec input
    63,     // thru
    63,     // loop
    63,     // output
    127};    // codec output defaults to 1.0



loopMachine::loopMachine() :
   AudioStream(LOOPER_CHANNELS,LOOPER_CHANNELS,inputQueueArray)
{
    LOG("ctor",0);
    pCodec = AudioCodec::getSystemCodec();
    assert(pCodec);
    
    for (int i=0; i<NUM_CONTROLS; i++)
    {
        m_control[i].value = 0;
        m_control[i].default_value = control_default[i];
        m_control[i].scale = 0.00;
        m_control[i].multiplier = 0;
    }
    for (int i=0; i<NUM_METERS; i++)
    {
        for (int j=0; j<LOOPER_CHANNELS; j++)
        {
            m_meter[i].min_sample[j] = 0;
            m_meter[i].max_sample[j] = 0;
        }
    }
    
    m_pLoopBuffer = new loopBuffer();
    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        m_tracks[i] = new loopTrack(i,this);
    }
    init();
    
    setUSBOpts();

    LOG("looper ctor finished",0);
}



loopMachine::~loopMachine()
{
    LOG("dtor",0);
    pCodec = 0;
    if (m_pLoopBuffer)
        delete m_pLoopBuffer;
    m_pLoopBuffer = 0;
    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        if (m_tracks[i])
            delete  m_tracks[i];
        m_tracks[i] = 0;
    }
    LOG("dtor finished",0);
}


void loopMachine::init()
{
    m_state = 0;
    m_pending_state = 0;
    m_cur_track_num = 0;
    m_selected_track_num = 0;
    m_pRecordClip = 0;
    
    m_pLoopBuffer->init();
    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        m_tracks[i]->init();
    }
}



// static
const char *loopMachine::getLoopStateName(u16 state)
{
    if (state == LOOP_STATE_NONE)         return "NONE";
    if (state == LOOP_STATE_RECORDING)    return "RECORDING";
    if (state == LOOP_STATE_PLAYING)      return "PLAYING";
    if (state == LOOP_STATE_STOPPED)      return "STOPPED";
    return "UNKNOWN_STATE";
}



// static
const char *loopMachine::getCommandName(u16 state)
{
    if (state == LOOP_COMMAND_NONE)                 return "";                // function is disabled
    if (state == LOOP_COMMAND_CLEAR_ALL)            return "CLEAR";
    if (state == LOOP_COMMAND_STOP)                 return "STOP";
    if (state == LOOP_COMMAND_PLAY)                 return "PLAY";
    if (state == LOOP_COMMAND_RECORD)               return "REC";
    if (state == LOOP_COMMAND_SELECT_NEXT_TRACK)    return "TRACK";
    if (state == LOOP_COMMAND_SELECT_NEXT_CLIP)     return "CLIP";
    if (state == LOOP_COMMAND_STOP_IMMEDIATE)       return "STOP!";

    return "UNKNOWN_STATE";
}



u16 loopMachine::getNumUsedTracks()
    // tracks are 'used' if the 0th clip
    // has any recorded content, or is recording
{
    u16 i = 0;
    u16 retval = 0;
    while (i < LOOPER_NUM_TRACKS &&
           m_tracks[i]->getClip(0)->getClipState() > LOOP_CLIP_STATE_EMPTY)
    {
        retval++;
        i++;
    }
    return retval;
}



//-------------------------------------------------
// controls and meters
//-------------------------------------------------


float loopMachine::getMeter(u16 meter, u16 channel)
{
    meter_t *pm = &m_meter[meter];
    int min = - pm->min_sample[channel];
    int max = pm->max_sample[channel];
    pm->max_sample[channel] = 0;
    pm->min_sample[channel] = 0;
    if (min > max) max = min;
    return (float)max / 32767.0f;
}


u8 loopMachine::getControlValue(u16 control)
{
    return m_control[control].value;
}
u8 loopMachine::getControlDefault(u16 control)
{
    return m_control[control].default_value;
}


void loopMachine::setControl(u16 control, u8 value)
{
    float scale = ((float)value)/127.00;
    if (control == CONTROL_INPUT_GAIN)
    {
        ((AudioControlCS42448 *)pCodec)->inputLevel(scale);
    }
    else if (control == CONTROL_OUTPUT_GAIN)
    {
        ((AudioControlCS42448 *)pCodec)->volume(scale);
    }
    else
    {
        #if WITH_INT_VOLUMES
            //if (n > 32767.0f) n = 32767.0f;
            //else if (n < -32767.0f) n = -32767.0f;
            m_control[control].multiplier = scale * 65536.0f;            
        #else
            scale = ((float)value)/63;
        #endif
    }
    m_control[control].value = value;
    m_control[control].scale = scale;
}


void  loopMachine::setPendingState(u16 state)
    // WHEREAS, it's ok to LOG the pending state change
    // because this is NOT called from update()
{
    m_pending_state = state;
    LOG("setPendingState(%s)",getLoopStateName(state));
}



void loopMachine::selectTrack(u16 num)
{
    m_selected_track_num = num;
}



//-------------------------------------------------
// command processor
//-------------------------------------------------

void loopMachine::command(u16 command, u16 param /*=0*/)
{
    switch (command)
    {
        case LOOP_COMMAND_CLEAR_ALL :
            {
                init();
                break;
            }
        case LOOP_COMMAND_STOP :
            {
                if (m_state == LOOP_STATE_PLAYING ||
                    m_state == LOOP_STATE_RECORDING)
                {
                    setPendingState(LOOP_STATE_STOPPED);
                }
                else
                {
                    LOG_WARNING("NO NEED TO STOP (should be different function)",0);
                }
                break;
            }
        case LOOP_COMMAND_STOP_IMMEDIATE :
            {
                if (m_state == LOOP_STATE_PLAYING ||
                    m_state == LOOP_STATE_RECORDING)
                {
                    m_state = LOOP_STATE_STOPPED;
                    m_pending_state = 0;
                    if (m_pRecordClip)
                    {
                        if (!m_pRecordClip->getClipNum())
                            m_pRecordClip->commit();
                        else
                            m_pRecordClip->init();
                        m_pRecordClip = 0;
                    }
                    getCurTrack()->zeroClips();
                }
                else
                {
                    LOG_WARNING("NO NEED TO STOP_IMMEDIATE (should be different function)",0);
                }
                break;
            }
        case LOOP_COMMAND_RECORD  :
            {
                loopTrack *pSelTrack = getSelectedTrack();
                u16 record_clip_num = pSelTrack->getNumClips();
                if (record_clip_num >= LOOPER_NUM_LAYERS)
                {
                    LOG_ERROR("No more clips available on track %d (record should be disabled)",pSelTrack->getTrackNum());
                    return;
                }
                setPendingState(LOOP_STATE_RECORDING);
                break;
            }
        case LOOP_COMMAND_PLAY    :
            {
                loopTrack *pSelTrack = getSelectedTrack();
                u16 num_clips = pSelTrack->getNumClips();
                if (!num_clips)
                {
                    LOG_ERROR("No clips available on track %d (play should be disabled)",pSelTrack->getTrackNum());
                    return;
                }
                setPendingState(LOOP_STATE_PLAYING);
                break;
            }
        case LOOP_COMMAND_SELECT_NEXT_TRACK :
            {
                LOG("SELECT_NEXT_TRACK(%d)",m_selected_track_num);
                
                setPendingState(LOOP_STATE_NONE);

                m_selected_track_num++;
                if (m_selected_track_num > getNumUsedTracks() ||
                    m_selected_track_num == LOOPER_NUM_TRACKS)
                    m_selected_track_num = 0;
                    
                if (m_selected_track_num != m_cur_track_num &&
                    m_state != LOOP_STATE_PLAYING &&
                    m_state != LOOP_STATE_RECORDING)
                {
                    LOG("changing cur_track_num to %d",m_selected_track_num);
                    m_cur_track_num = m_selected_track_num;
                }
                break;
            }
        case LOOP_COMMAND_SELECT_NEXT_CLIP :
            {
                loopTrack *pTrack = getTrack(getSelectedTrackNum());
                u16 clip_num = pTrack->getSelectedClipNum();
                LOG("SELECT_NEXT_CLIP(%d)",clip_num);
                
                clip_num++;
                if (clip_num > pTrack->getNumClips() ||
                    clip_num == LOOPER_NUM_LAYERS)
                    clip_num = 0;
                pTrack->setSelectedClipNum(clip_num);
                break;
            }
        default:
            LOG_ERROR("unimplemented command(%d)",command);
            break;
            
    }
}




// virtual
void loopMachine::update(void)
{
    // always receive any input blocks
    // which in our case will always have content

    s16 *ip[LOOPER_CHANNELS];
	audio_block_t *in[LOOPER_CHANNELS];
	for (u16 channel=0; channel<LOOPER_CHANNELS; channel++)
    {
        in[channel] = receiveWritable(channel);
        ip[channel] = in[channel] ? in[channel]->data : 0;
    }
    
    // Handle the pending state change, if any
    
    loopTrack *pCurTrack = getCurTrack();
    if (m_pending_state)
    {
        // determine if we should keep rolling ...
        // ALL are immediate if we are stopped or none
        // STOPS are are immediate if we are not recording,
        // everything else is immediate if we are recording the 0th clip
        // otherwise, finally, we wait till the base track is at zero
        
        bool doit = false;
        if (m_state == LOOP_STATE_NONE || m_state == LOOP_STATE_STOPPED)
            doit = true;
        // else if (m_state == LOOP_STATE_PLAYING && m_pending_state == LOOP_STATE_STOPPED)
        //     doit = true;
        else if (m_pRecordClip && !m_pRecordClip->getClipNum())
            doit = true;
        else if (!pCurTrack->getClip(0)->getCurBlock())
            doit = true;
        
        
        if (doit)
        {
            // DO NOT USE LOG in an interrupt routine and expect it to work correctly!
            // LOG("handling pending change to %d",m_pending_state);
        
            // if we are recording, commit it ...
            
            if (m_pRecordClip)
            {
                m_pRecordClip->commit();
                m_pRecordClip = 0;
            }
        
            // if the selected track is not the same as the current track,
            // change it
            
            if (m_selected_track_num != m_cur_track_num)
            {
                pCurTrack->zeroClips();
                m_cur_track_num = m_selected_track_num;
                pCurTrack = getCurTrack();
            }
            
            // if switching to recording, get the next available clip
            // and start it. It is assumed nobody has told us to do
            // anything illegal, like record on an unavailable clip
            // or track!
            
            if (m_pending_state == LOOP_STATE_RECORDING)
            {
                u16 record_clip_num = pCurTrack->getNumClips();
                m_pRecordClip = pCurTrack->getClip(record_clip_num);
                m_pRecordClip->start();
            }
            
            // change to the new state
            // DO NOT WRITE TO THE SERIAL PORT, duh!
            
            m_state = m_pending_state;
            m_pending_state = 0;
        }
    }

    
    // setup pointers
    
    s16 *clip_ptr[LOOPER_NUM_LAYERS];
    u16 num_clips = pCurTrack->getNumRecordedClips();
    bool playing =
        (m_state == LOOP_STATE_RECORDING) ||
        (m_state == LOOP_STATE_PLAYING);
    
    if (playing)
    {
        for (int clip_num=0; clip_num<num_clips; clip_num++)
        {
            loopClip *pClip = pCurTrack->getClip(clip_num);    // get the playback clip
            clip_ptr[clip_num] = pClip->getBlockBuffer();
        }
    }

    s16 *rec_ptr = m_state == LOOP_STATE_RECORDING ?
        m_pRecordClip->getBlockBuffer() : 0;

    #if WITH_VOLUMES
        #if WITH_INT_VOLUMES
            int32_t thru_mult = m_control[CONTROL_THRU_VOLUME].multiplier;
            int32_t loop_mult = m_control[CONTROL_LOOP_VOLUME].multiplier;
            int32_t mix_mult = m_control[CONTROL_MIX_VOLUME].multiplier;
        #else
            float thru_level = m_control[CONTROL_THRU_VOLUME].scale;
            float loop_level = m_control[CONTROL_LOOP_VOLUME].scale;
            float mix_level = m_control[CONTROL_MIX_VOLUME].scale;
        #endif
    #endif
    
    // loop through two channels of 128 samples
    
	for (u16 channel=0; channel<LOOPER_CHANNELS; channel++)
    {
        s16 *in_ptr = ip[channel] ? ip[channel] : 0;
        
        s16 *in_max   = &(m_meter[METER_INPUT].max_sample[channel]  );
        s16 *in_min   = &(m_meter[METER_INPUT].min_sample[channel]  );
        s16 *thru_max = &(m_meter[METER_THRU].max_sample[channel]   );
        s16 *thru_min = &(m_meter[METER_THRU].min_sample[channel]   );
        s16 *loop_max = &(m_meter[METER_LOOP].max_sample[channel]   );
        s16 *loop_min = &(m_meter[METER_LOOP].min_sample[channel]   );
        s16 *mix_max  = &(m_meter[METER_MIX].max_sample[channel] );
        s16 *mix_min  = &(m_meter[METER_MIX].min_sample[channel] );
        
        
        for (u16 i=0; i<AUDIO_BLOCK_SAMPLES; i++)
        {
            // input
            
            s16 val = in_ptr ? *in_ptr : 0;
            if (rec_ptr)
                *rec_ptr++ = val;
                
            if (val > *in_max)
                *in_max = val;
            if (val <*in_min)
                *in_min = val;
            
            // thru
            
            #if WITH_VOLUMES
                #if WITH_INT_VOLUMES
                    applyGain(&val,thru_mult);
                #else                    
                    val = ((float)val) * thru_level;
                #endif
            #endif

            if (val > *thru_max)
                *thru_max = val;
            if (val < *thru_min)
                *thru_min = val;
            
            // loop
            
            if (playing)
            {
                s16 loop_val = 0;
                for (u16 clip_num=0; clip_num<num_clips; clip_num++)
                {
                    loop_val += *clip_ptr[clip_num]++;
                }

                #if WITH_VOLUMES
                    #if WITH_INT_VOLUMES
                        applyGain(&loop_val,loop_mult);
                    #else
                        loop_val = ((float)loop_val)*loop_level;                    
                    #endif
                #endif
                
                val += loop_val;

                if (loop_val > *loop_max)
                    *loop_max = loop_val;
                if (loop_val < *loop_min)
                    *loop_min = loop_val;
            }
            
            // mix
            
            #if WITH_VOLUMES
                #if WITH_INT_VOLUMES
                    applyGain(&val,mix_mult);
                #else                    
                    val = ((float)val) * mix_level;
                #endif
            #endif

            if (val > *mix_max)
                *mix_max = val;
            if (val < *mix_min)
                *mix_min = val;
            
            *in_ptr++ = val;
        }
    }
    
    // increment block numbers

    if (m_pRecordClip)
        m_pRecordClip->incCurBlock();   
    
    if (playing)
    {
        for (int clip_num=0; clip_num<num_clips; clip_num++)
        {
            pCurTrack->getClip(clip_num)->incCurBlock();   
        }
    }
    
    
    // transmit the output blocks
    
	for (u16 channel=0; channel<LOOPER_CHANNELS; channel++)
    {
        if (ip[channel])
        {
            transmit(in[channel], channel);
            AudioSystem::release(in[channel]);
        }
    }

}






