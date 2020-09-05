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

const char *getLoopStateName(u16 state)
{
    if (state == LOOP_STATE_NONE)         return "NONE";
    if (state == LOOP_STATE_RUNNING)      return "RUNNING";
    if (state == LOOP_STATE_STOPPING)     return "STOPPING";
    if (state == LOOP_STATE_STOPPED)      return "STOPPED";
    return "UNKNOWN_LOOP_STATE";
}

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



float publicLoopMachine::getMeter(u16 meter, u16 channel)
{
    // technically, interrupts should be turned off for this
    // and possible volatile declared here or there.
    meter_t *pm = &m_meter[meter];
    int min = - pm->min_sample[channel];
    int max = pm->max_sample[channel];
    pm->max_sample[channel] = 0;
    pm->min_sample[channel] = 0;
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

    m_prev_track_num = -1;
    m_cur_track_num = -1;
    m_num_used_tracks = 0;

    pTheLoopBuffer->init();

    for (int i=0; i<LOOPER_NUM_TRACKS; i++)
    {
        m_tracks[i]->init();
    }
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


void  loopMachine::setPendingCommand(u16 command)
    // it's ok to LOG the pending command change
    // because this is NOT called from update()
{
    m_pending_command = command;
    #if DEBUG_LOOPER_UPDATE
        LOG("setPendingCommand(%s)",getLoopCommandName(command));
    #endif
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
            if (pTrack->getNumUsedClips())
                retval = true;
            break;
        case LOOP_COMMAND_RECORD:
            if (!clip_state)
                retval = true;
            break;
        case LOOP_COMMAND_STOP:
            if (m_loop_state == LOOP_STATE_RUNNING)
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
            if (m_loop_state == LOOP_STATE_RUNNING ||
                m_loop_state == LOOP_STATE_STOPPING)
                retval = true;
            break;
        case LOOP_COMMAND_CLEAR_ALL:
            if (m_loop_state)
                retval = true;
            break;
    }

    return retval;
}



// virtual
void loopMachine::command(u16 command)
{
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
            setPendingCommand(LOOP_COMMAND_NONE);
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
            setPendingCommand(LOOP_COMMAND_NONE);
        }
        else
        {
            setPendingCommand(command);
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
        #else
            float thru_level = m_control[CONTROL_THRU_VOLUME].scale;
            float loop_level = m_control[CONTROL_LOOP_VOLUME].scale;
        #endif
    #endif

    audio_block_t *in[LOOPER_NUM_CHANNELS];
    audio_block_t *out[LOOPER_NUM_CHANNELS];
    for (u16 channel=0; channel<LOOPER_NUM_CHANNELS; channel++)
    {
        in[channel] = receiveReadOnly(channel);                 // will always exist
        out[channel] = AudioSystem::allocate();

        s16 *ip = in[channel] ? in[channel]->data : 0;
        s16 *op = out[channel]->data;

        #if WITH_METERS
            s16 *in_max   = &(m_meter[METER_INPUT].max_sample[channel]  );
            s16 *in_min   = &(m_meter[METER_INPUT].min_sample[channel]  );
            s16 *thru_max = &(m_meter[METER_THRU].max_sample[channel]   );
            s16 *thru_min = &(m_meter[METER_THRU].min_sample[channel]   );
        #endif

        // 1st loop through samples

        for (u16 i=0; i<AUDIO_BLOCK_SAMPLES; i++)
        {
            s16 val = ip ? *ip++ : 0;

            // InputMeter min and max

            #if WITH_METERS
                if (val > *in_max)
                    *in_max = val;
                if (val <*in_min)
                    *in_min = val;
            #endif

            // apply ThruControl volume level

            #if WITH_VOLUMES
                #if WITH_INT_VOLUMES
                    applyGain(&val,thru_mult);
                #else
                    val = ((float)val) * thru_level;
                #endif
            #endif

            // ThruMeter min and max

            #if WITH_METERS
                if (val > *thru_max)
                    *thru_max = val;
                if (val < *thru_min)
                    *thru_min = val;
            #endif

            // place the sample in the output buffer

            *op++ = val;

        }   // for each input sample
    }   // for each channel


    //-------------------
    // update the state
    //-------------------
    // and if the machine is still running,
    // call update() on the previous and/or current track

    if (in[0])
    {
        updateState();

        if (m_loop_state == LOOP_STATE_RUNNING ||
            m_loop_state == LOOP_STATE_STOPPING)
        {
            loopTrack *pPrevTrack = m_prev_track_num >= 0 ? getTrack(m_prev_track_num) : 0;
            loopTrack *pCurTrack = m_cur_track_num >= 0 ? getTrack(m_cur_track_num) : 0;

            if (pPrevTrack)
            {
                // LOG("calling pPrevTrack->update()",0);
                pPrevTrack->update(in,out);
            }
            if (pCurTrack && pCurTrack != pPrevTrack)
            {
                // LOG("calling pCurTrack->update()",0);
                pCurTrack->update(in,out);
            }

        }   // LOOP_STATE_RUNNING or LOOP_STATE_STOPPING
    }   // in[0] (audioSystem is started)

    //-------------------------------------
    // transmit the output blocks
    //-------------------------------------
    // and release the input blocks

    for (u16 channel=0; channel<LOOPER_NUM_CHANNELS; channel++)
    {
        transmit(out[channel], channel);
        if (in[channel])
            AudioSystem::release(in[channel]);
        AudioSystem::release(out[channel]);
    }

}   // loopMachine::update()



//--------------------------------------------------------------------------------
// updateState()
//--------------------------------------------------------------------------------

void loopMachine::setLoopState(u16 loop_state)
{
    #if DEBUG_LOOPER_UPDATE
        LOG("LOOP_STATE_%s",getLoopStateName(loop_state));
    #endif
    m_loop_state = loop_state;
}
void loopMachine::setCurTrackNum(int num)
{
    #if DEBUG_LOOPER_UPDATE
        LOG("cur_track_num(%d) ----> %d",m_cur_track_num,num);
    #endif
    m_cur_track_num = num;
}
void loopMachine::setPrevTrackNum(int num)
{
    #if DEBUG_LOOPER_UPDATE
        LOG("prev_track_num(%d) ----> %d",m_prev_track_num,num);
    #endif
    m_prev_track_num = num;
}
void loopMachine::incDecNumUsedTracks(int inc)
{
    m_num_used_tracks += inc;
    #if DEBUG_LOOPER_UPDATE
        LOG("num_used_tracks(%d) --> %d",inc,m_num_used_tracks);
    #endif
}




void loopMachine::updateState(void)
{
    loopTrack *pSelTrack = getTrack(m_selected_track_num);
    loopTrack *pCurTrack = m_cur_track_num >= 0 ?  getTrack(m_cur_track_num) : 0;
    loopTrack *pPrevTrack = m_prev_track_num >= 0 ?  getTrack(m_prev_track_num) : 0;
    loopClip *pSelClip = pSelTrack->getSelectedClip();
    u16 sel_clip_state = pSelClip->getClipState();

    // filter out obvious bad commands
    // can only issue RECORD command in LOOP_STATE_NONE

    if (m_pending_command &&
        m_loop_state == LOOP_STATE_NONE)
    {
        if (m_pending_command != LOOP_COMMAND_RECORD)
        {
            LOG_WARNING("attempt to issue command(%s) other than RECORD while in LOOP_STATE_NONE",getLoopCommandName(m_pending_command));
            m_pending_command = 0;
            return;
        }
    }

    // verify that PLAY and RECORD commands have valid selected clips

    else
    {
        if (m_pending_command == LOOP_COMMAND_PLAY)
        {
            if (!pSelTrack->getNumRecordedClips())
            {
                LOG_WARNING("attempt to issue command(PLAY) on empty track(%d)",m_selected_track_num);
                m_pending_command = 0;
                return;
            }
        }
        else if (m_pending_command == LOOP_COMMAND_RECORD)
        {
            if (sel_clip_state != CLIP_STATE_NONE)
            {
                CString *state_msg = getClipStateName(sel_clip_state);
                LOG_WARNING("attempt to issue command(RECORD) on clip(%d,%d) which is in state(%s)",
                    m_selected_track_num,
                    pSelTrack->getSelectedClipNum(),
                    (const char *)*state_msg);
                delete state_msg;
                m_pending_command = 0;
                return;
            }
        }

        // any command besides PLAY or RECORD is illegal in state STOPPED

        else if (m_loop_state == LOOP_STATE_STOPPED &&
                 m_pending_command &&
                 m_pending_command != LOOP_COMMAND_PLAY &&
                 m_pending_command != LOOP_COMMAND_RECORD)
        {
            LOG_WARNING("attempt to issue command(%s) while in LOOP_STATE_STOPPED",getLoopCommandName(m_pending_command));
            m_pending_command = 0;
            return;
        }
    }

    // handle STOP_IMMEDIATE
    // bring everything to a screeching halt

    if (m_pending_command  == LOOP_COMMAND_STOP_IMMEDIATE)
    {
        LOG("LOOP_COMMAND_STOP_IMMEDIATE",0);

        if (pPrevTrack)
            pPrevTrack->stopImmediate();
        if (pCurTrack && pCurTrack != pPrevTrack)
            pCurTrack->stopImmediate();
        if (pSelTrack != pCurTrack && pSelTrack != pPrevTrack)
            pSelTrack->stopImmediate();

        setLoopState(LOOP_STATE_STOPPED);
            // needs to be NONE if was recording clip(0,0)

        m_cur_track_num = -1;
        m_prev_track_num = -1;
    }

    // if stopping, we set the loop state here
    // and rely on the tracks and clips to stop
    // the looper

    else if (m_pending_command == LOOP_COMMAND_STOP)
    {
        setLoopState(LOOP_STATE_STOPPING);
    }


    // pass the updateState() to the prev_track if any,
    // then to the current track, if any, then finally
    // to the selected track, if any.  Only call updateState()
    // once on any track.  We pass the loop_state and pending
    // command up, as well as telling the track "how" it is
    // begin called.

    if (pPrevTrack)
        pPrevTrack->updateState(UPDATE_STATE_CALLED_AS_PREV,m_loop_state, m_pending_command);
    if (pCurTrack && pCurTrack != pPrevTrack)
        pCurTrack->updateState(UPDATE_STATE_CALLED_AS_CUR,m_loop_state, m_pending_command);
    if (pSelTrack != pCurTrack && pSelTrack != pPrevTrack)
        pSelTrack->updateState(UPDATE_STATE_CALLED_AS_SELECTED,m_loop_state, m_pending_command);


    if (m_pending_command == LOOP_COMMAND_STOP)
    {
        setPendingCommand(LOOP_COMMAND_NONE);
    }


}


