#include "Looper.h"
#include <circle/logger.h>
#include <circle/synchronize.h>

#define log_name "lmachine"

#define WITH_VOLUMES       0
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



// The default input gain for the cs42448 of 0db
// 0.723 = (128.0 / (128.0 + 1.0 + 48.0))
// 91.84 =  0.723 * 127
// My volumes are 100 based to allow for gain
// The output gain is limited to 1.0

u16 control_default[NUM_CONTROLS] = {
    94,     // codec input
    63,     // thru
    63,     // loop
    63,     // output
    127};    // codec output defaults to 1.0


//-------------------------------------------
// static externs
//-------------------------------------------


const char *getLoopCommandName(u16 state)
{
    if (state == LOOP_COMMAND_NONE)                 return "";                // function is disabled
    if (state == LOOP_COMMAND_STOP_IMMEDIATE)       return "STOP!";
    if (state == LOOP_COMMAND_CLEAR_ALL)            return "CLEAR";
    if (state == LOOP_COMMAND_STOP)                 return "STOP";
    if (state == LOOP_COMMAND_PLAY)                 return "PLAY";
    if (state == LOOP_COMMAND_RECORD)               return "REC";
    if (state == LOOP_COMMAND_SELECT_NEXT_TRACK)    return "TRACK";
    if (state == LOOP_COMMAND_SELECT_NEXT_CLIP)     return "CLIP";
    return "UNKNOWN_LOOP_COMMAND";
}


//-------------------------------------------
// publicLoopMachine
//-------------------------------------------

publicLoopMachine::publicLoopMachine() :
   AudioStream(LOOPER_NUM_CHANNELS,LOOPER_NUM_CHANNELS,inputQueueArray)
{
    LOG("publicLoopMachine ctor",0);
    pCodec = AudioCodec::getSystemCodec();
    assert(pCodec);

    m_pFirstLogString = 0;
    m_pLastLogString = 0;

    for (int i=0; i<NUM_CONTROLS; i++)
    {
        m_control[i].value = 0;
        m_control[i].default_value = control_default[i];
        m_control[i].scale = 0.00;
        m_control[i].multiplier = 0;
    }
    for (int i=0; i<NUM_METERS; i++)
    {
        for (int j=0; j<LOOPER_NUM_CHANNELS; j++)
        {
            m_meter[i].min_sample[j] = 0;
            m_meter[i].max_sample[j] = 0;
        }
    }

    init();

    LOG("publicLoopMachine ctor finished",0);
}



publicLoopMachine::~publicLoopMachine()
{
    pCodec = 0;
}




logString_t *publicLoopMachine::getNextLogString()
    // hand out the head of the list
{
	DisableIRQs();	// in synchronize.h
    logString_t *retval =  m_pFirstLogString;
    if (retval)
    {
        m_pFirstLogString = retval->next;
        retval->next = 0;   // for safety
    }
    if (!m_pFirstLogString)
        m_pLastLogString = 0;
    EnableIRQs();
    return retval;
}


float publicLoopMachine::getMeter(u16 meter, u16 channel)
{
    // technically, interrupts should be turned off for this
    // and possible volatile declared here or there.
    meter_t *pm = &m_meter[meter];
	DisableIRQs();	// in synchronize.h
    int min = - pm->min_sample[channel];
    int max = pm->max_sample[channel];
    pm->max_sample[channel] = 0;
    pm->min_sample[channel] = 0;
    EnableIRQs();
    if (min > max) max = min;
    return (float)max / 32767.0f;
}


u8 publicLoopMachine::getControlValue(u16 control)
{
    return m_control[control].value;
}

u8 publicLoopMachine::getControlDefault(u16 control)
{
    return m_control[control].default_value;
}


void publicLoopMachine::setControl(u16 control, u8 value)
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



//--------------------------------------------------
// loopMachine
//--------------------------------------------------

loopMachine::loopMachine() : publicLoopMachine()
{
    LOG("loopMachine ctor",0);

    new loopBuffer();

    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        m_tracks[i] = new loopTrack(i);
    }

    init();

    LOG("loopMachine ctor finished",0);
}



loopMachine::~loopMachine()
{
    if (pTheLoopBuffer)
        delete pTheLoopBuffer;
    pTheLoopBuffer = 0;
    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        if (m_tracks[i])
            delete  m_tracks[i];
        m_tracks[i] = 0;
    }
}


void loopMachine::init()
    // initializes base class members too
{
    publicLoopMachine::init();

    m_cur_command = 0;
    m_cur_track_num = -1;
    m_num_used_tracks = 0;

    pTheLoopBuffer->init();

    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        m_tracks[i]->init();
    }
}




void loopMachine::LogUpdate(const char *lname, const char *format, ...)
{
	va_list vars;
	va_start(vars, format);

    #if 0
        CString *msg;
        msg.FormatV(format,vars);
        va_end (vars);
        CLogger::Get()->Write(lname,LogDebug,msg);
    #else

        logString_t *pMem = new logString_t;
        pMem->next = 0;
        pMem->lname = lname;
        pMem->string = new CString();
        pMem->string->FormatV(format,vars);
        va_end (vars);

        if (!m_pFirstLogString)
            m_pFirstLogString = pMem;
        if (m_pLastLogString)
            m_pLastLogString->next = pMem;

        m_pLastLogString = pMem;

    #endif
}



//-------------------------------------------------
// command processor
//-------------------------------------------------


void loopMachine::selectTrack(u16 num)
{
    if (num != m_selected_track_num)
    {
        m_tracks[m_selected_track_num]->setSelected(false);
        m_selected_track_num = num;
        m_tracks[m_selected_track_num]->setSelected(true);
    }
}




// virtual
bool loopMachine::canDo(u16 command)
{
    bool retval = 0;
	loopTrack *pTrack = getTrack(m_selected_track_num);
	loopClip *pClip = pTrack->getSelectedClip();
	u16 clip_state = pClip->getClipState();

	switch (command)
    {
        case LOOP_COMMAND_PLAY:
            if (pTrack->getNumUsedClips() &&
                !(clip_state & CLIP_STATE_PLAY_MAIN))
                retval = true;
            break;
        case LOOP_COMMAND_RECORD:
            if (!clip_state)
                retval = true;
            break;
        case LOOP_COMMAND_STOP:
            if (m_running && m_num_used_tracks)
                retval = true;
            break;
        case LOOP_COMMAND_SELECT_NEXT_CLIP:
            if (pTrack->getNumUsedClips())
                return true;
            break;
        case LOOP_COMMAND_SELECT_NEXT_TRACK:
            if (m_num_used_tracks)
                return true;
            break;
        case LOOP_COMMAND_STOP_IMMEDIATE:
            if (m_running)
                retval = true;
            break;
        case LOOP_COMMAND_CLEAR_ALL:
            if (!m_running && m_num_used_tracks)
                retval = true;
            break;
    }

    return retval;
}



// virtual
void loopMachine::command(u16 command)
{
    if (command == LOOP_COMMAND_NONE)
        return;

    LOG("loopMachine::command(%s)",getLoopCommandName(command));

    if (canDo(command))
    {
        if (command == LOOP_COMMAND_CLEAR_ALL)
        {
            LOG("LOOP_COMMAND_CLEAR",0);
            init();
        }
        else if (command == LOOP_COMMAND_SELECT_NEXT_TRACK)
        {
            u16 new_track_num = m_selected_track_num+1;
            if (new_track_num > m_num_used_tracks ||
                new_track_num == LOOPER_NUM_TRACKS)
                new_track_num = 0;
            if (new_track_num != m_selected_track_num)
            {
                m_tracks[m_selected_track_num]->setSelected(false);
                m_selected_track_num = new_track_num;
                m_tracks[m_selected_track_num]->setSelected(true);
            }
            LOG("SELECT_NEXT_TRACK(%d)",m_selected_track_num);
            m_pending_command = 0;
        }
        else if (command == LOOP_COMMAND_SELECT_NEXT_CLIP)
        {
            loopTrack *pTrack = getTrack(m_selected_track_num);
            u16 clip_num = pTrack->getSelectedClipNum();
            clip_num++;
            if (clip_num > pTrack->getNumUsedClips() ||
                clip_num == LOOPER_NUM_LAYERS)
                clip_num = 0;
            LOG("SELECT_NEXT_CLIP(%d)",clip_num);
            pTrack->setSelectedClipNum(clip_num);
            m_pending_command = 0;
        }
        else
        {
            m_pending_command = command;
            LOG("PENDING_COMMAND(%s)",getLoopCommandName(m_pending_command));
        }
    }
    else
        LOG_WARNING("loopMachine cannot do command %s at this time",getLoopCommandName(command));

}   // loopMachine::command()




//----------------------------------------------
// Update()
//----------------------------------------------

// virtual
void loopMachine::update(void)
{
    m_cur_command = 0;
        // the current command is a short-lived member variable,
        // only valid for the duration of update.  It is "latched"
        // from the pending command at "loopPoints".

    //--------------------------------------------------------------
    // receive the input audio blocks and create the output blocks
    //--------------------------------------------------------------
    // loop through the samples setting the InputMeter min max,
    // applying the ThruControl, set the ThruMeter min and max,
    // and put the result in the output block.

    #if WITH_VOLUMES
        #if WITH_INT_VOLUMES
            int32_t thru_mult = m_control[CONTROL_THRU_VOLUME].multiplier;
            int32_t loop_mult = m_control[CONTROL_LOOP_VOLUME].multiplier;
            int32_t mix_mult = m_control[CONTROL_MIX_VOLUME].multiplier;
        #else
            float thru_level = m_control[CONTROL_THRU_VOLUME].scale;
            float loop_level = m_control[CONTROL_LOOP_VOLUME].scale;
            float mix_level =  m_control[CONTROL_MIX_VOLUME].scale;
        #endif
    #endif

    // Set the input buffer and create empty output buffers

    audio_block_t *in[LOOPER_NUM_CHANNELS];
    audio_block_t *out[LOOPER_NUM_CHANNELS];
    for (u16 channel=0; channel<LOOPER_NUM_CHANNELS; channel++)
    {
        in[channel] = receiveReadOnly(channel);
        out[channel] = AudioSystem::allocate();
        memset(out[channel]->data,0,AUDIO_BLOCK_BYTES);

        // a preliminary loop through the input buffer
        // JUST to set the input meter

        #if WITH_METERS
            s16 *ip = in[channel] ? in[channel]->data : 0;
            s16 *in_max   = &(m_meter[METER_INPUT].max_sample[channel]  );
            s16 *in_min   = &(m_meter[METER_INPUT].min_sample[channel]  );
            for (u16 i=0; i<AUDIO_BLOCK_SAMPLES; i++)
            {
                s16 val = ip ? *ip++ : 0;
                if (val > *in_max)
                    *in_max = val;
                if (val <*in_min)
                    *in_min = val;
            }
        #endif

    }   // for each channel

    //-------------------
    // update the state
    //-------------------
    // and if the machine is still running,
    // call update() on any running tracks

    if (in[0])
    {
        updateState();

        if (m_running)
        {
            for (int i=0; i<m_num_used_tracks; i++)
            {
                loopTrack *pTrack = getTrack(i);
                if (pTrack->getNumRunningClips())
                    pTrack->update(in,out);
            }
        }   // m_running

    }   // in[0] (audioSystem is started)


    //-----------------------------------------------
    // final mix of in ==> thru ==> out
    //-----------------------------------------------
    // and transmitting and releasing of audio blocks
    // out[channel] contains combined "loop" values
    // in[channel] contains the as-yet-unmixed "thru" values

    for (u16 channel=0; channel<LOOPER_NUM_CHANNELS; channel++)
    {
        s16 *ip = in[channel] ? in[channel]->data : 0;
        s16 *op = out[channel]->data;

        #if WITH_METERS
            s16 *thru_max   = &(m_meter[METER_THRU].max_sample[channel]);
            s16 *thru_min   = &(m_meter[METER_THRU].min_sample[channel]);
            s16 *loop_max   = &(m_meter[METER_LOOP].max_sample[channel]);
            s16 *loop_min   = &(m_meter[METER_LOOP].min_sample[channel]);
            s16 *mix_max    = &(m_meter[METER_MIX].max_sample[channel]);
            s16 *mix_min    = &(m_meter[METER_MIX].min_sample[channel]);
        #endif

        for (u16 i=0; i<AUDIO_BLOCK_SAMPLES; i++)
        {
            s16 ival = ip ? *ip++ : 0;
            s16 oval = *op;                 // pointer not incremented

            // apply Thru and Loop Control volume level

            #if WITH_VOLUMES
                #if WITH_INT_VOLUMES
                    applyGain(&ival,thru_mult);
                    applyGain(&oval,loop_mult);
                #else
                    ival = ((float)ival) * thru_level;
                    oval = ((float)oval) * loop_level;
                #endif
            #endif

            // add them to create the Mix value
            // and apply it's volume if'defd

            s16 mval = ival + oval;
            #if WITH_VOLUMES
                #if WITH_INT_VOLUMES
                    applyGain(&mval,mix_mult);
               #else
                    mval = ((float)mval) * mix_level;
                #endif
            #endif

            // update the meters

            #if WITH_METERS
                if (ival > *thru_max)
                    *thru_max = ival;
                if (ival < *thru_min)
                    *thru_min = ival;
                if (oval > *loop_max)
                    *loop_max = oval;
                if (oval <*loop_min)
                    *loop_min = oval;
                if (mval > *mix_max)
                    *mix_max = ival;
                if (mval <*mix_min)
                    *mix_min = mval;
            #endif

            // place the sample in the output buffer

            *op++ = mval;

        }   // for each input sample

        // transmit the output blocks
        // and release all blocks

        transmit(out[channel], channel);
        if (in[channel])
            AudioSystem::release(in[channel]);
        AudioSystem::release(out[channel]);
    }

    m_cur_command = 0;
        // end of short lived command variable

}   // loopMachine::update()



//--------------------------------------------------------------------------------
// updateState()
//--------------------------------------------------------------------------------

void loopMachine::incDecNumUsedTracks(int inc)
{
    m_num_used_tracks += inc;
    LOOPER_LOG("num_used_tracks(%d) --> %d",inc,m_num_used_tracks);
}

void loopMachine::incDecRunning(int inc)
{
    m_running += inc;
    LOOPER_LOG("m_running=%d",m_running);
}



void loopMachine::updateState(void)
{
    // handle STOP_IMMEDIATE
    // bring everything to a screeching halt

    if (m_pending_command == LOOP_COMMAND_STOP_IMMEDIATE)
    {
        for (int i=0; i<m_num_used_tracks; i++)
            getTrack(i)->stopImmediate();

        m_running = 0;
        m_cur_track_num = -1;
        m_pending_command = 0;

        return;
    }

    //----------------------------------------
    // updateState() COMMAND processor.
    //----------------------------------------
    // Determine if we are at a "loop point" or a point
    // at which we should latch the pending command into
    // the current command ...

    if (m_pending_command)
    {
        loopTrack *pCurTrack = m_cur_track_num >= 0 ?  getTrack(m_cur_track_num) : 0;
        loopTrack *pSelTrack = getTrack(m_selected_track_num);
        loopClip  *pClip0 = pCurTrack ? pCurTrack->getClip(0) : 0;
        u16 clip0_state = pClip0 ? pClip0->getClipState() : 0;
            // the current base clip, and it's state, if any

        bool at_loop_point = (clip0_state & CLIP_STATE_PLAY_MAIN) && !pClip0->getPlayBlockNum();
        bool latch_command =
            !m_running ||
            at_loop_point ||
            !clip0_state ||
            !m_num_used_tracks ||
            (clip0_state & CLIP_STATE_RECORD_MAIN);

        // LATCH IN A NEW COMMAND

        if (latch_command)
        {
            m_cur_command = m_pending_command;
            m_pending_command = 0;

            LOOPER_LOG("latching pending command(%s)",getLoopCommandName(m_cur_command));

            // the previous track is entirely handled in update().
            // if we are changing tracks(), STOP the old track ...
            // and start the new one with the given command

            if (pCurTrack && pCurTrack != pSelTrack)
                pCurTrack->updateState(LOOP_COMMAND_STOP);
            pSelTrack->updateState(m_cur_command);

            // change the current track to the selected track

            if (m_cur_track_num != m_selected_track_num)
            {
                LOOPER_LOG("change m_cur_track_num(%d) to selected_track_num(%d)",m_cur_track_num,m_selected_track_num);
                m_cur_track_num = m_selected_track_num;
            }
        }

    }   // if m_pending_command
}   // loopMachine::updateState()
