// Defines that are common between rPi Looper and Arduino TeensyExpression projects
// are kept here in the Looper project

#ifndef __common_defines_h__
#define __common_defines_h__


//---------------------------------------------
// Previously from rPi Looper.h
//---------------------------------------------

#define TRACK_STATE_EMPTY               0x0000
#define TRACK_STATE_RECORDING           0x0001
#define TRACK_STATE_PLAYING             0x0002
#define TRACK_STATE_STOPPED             0x0004
#define TRACK_STATE_PENDING_RECORD      0x0008
#define TRACK_STATE_PENDING_PLAY        0x0010
#define TRACK_STATE_PENDING_STOP        0x0020
#define TRACK_STATE_PENDING			    (TRACK_STATE_PENDING_RECORD | TRACK_STATE_PENDING_PLAY | TRACK_STATE_PENDING_STOP)

#define LOOP_COMMAND_NONE               0x00
#define LOOP_COMMAND_CLEAR_ALL          0x01
#define LOOP_COMMAND_STOP_IMMEDIATE     0x02      // stop the looper immediately
#define LOOP_COMMAND_STOP               0x03      // stop at next cycle point
#define LOOP_COMMAND_DUB_MODE           0x08      // the dub mode is handled by rPi and modeled here
#define LOOP_COMMAND_TRACK_BASE         0x10      // the seven possible "track" buttons are 0x10..0x17
        // the above commands can be sent to the loop machine.
        // the following are for internal "pending" command use only
#define LOOP_COMMAND_RECORD             0x40
#define LOOP_COMMAND_PLAY               0x50


// Looper Serial CC numbers             // TE       rPi         descrip
#define LOOP_CONTROL_BASE_CC   0x65     // send     recv        for 0..LOOPER_NUM_CONTROLS the value is the volume control (Looper pedal == 0x67)
#define LOOP_STOP_CMD_STATE_CC 0x26		// recv     send        the value is 0, LOOP_COMMAND_STOP or STOP_IMMEDIATE
#define LOOP_DUB_STATE_CC      0x25		// recv     send        value is currently only the DUB state
#define LOOP_COMMAND_CC        0x24		// send     recv        the value is the LOOP command
#define TRACK_STATE_BASE_CC    0x14		// recv     send        for 0..3 tracks, value is track state
#define CLIP_VOL_BASE_CC       0x30		// send     recv        for 4 tracks * 3 clips - value is volume 0..127
#define CLIP_MUTE_BASE_CC      0x40		// both     both        for 4 tracks * 3 clips - value is mute state


#endif
