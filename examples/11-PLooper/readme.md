Programmable Looper
===================

This looper is designed to be very flexible,
yet simple to use in live performance situations.

It is programmed in terms of scripts, using a defined language,
that in turn, define the user interface and behavior of the looper.


Simple Example
--------------

This example script will program a three button looper.
The rightmost button will be STOP ALL, the one next to
it will be CLEAR ALL, and the left most button will act
to perform a simple sequence of operations:

1. on the first press, start recording
2. on the second press, start playing the track
3. on subsequent presses, toggle between playing
   and stopping the clip.
    
```

# HEADER
# define button1-4 to activate if a midi NOTE_ON event
# on any cable (-1), on midi channel 7, note number 32-35,
# with any velocity, is received.  We will only be using
# buttons 1, 3, and 4 in this example.

DEFINE_CONTROL(button1,-1,7,NOTE_ON,32,-1)
DEFINE_CONTROL(button2,-1,7,NOTE_ON,33,-1)
DEFINE_CONTROL(button3,-1,7,NOTE_ON,34,-1)
DEFINE_CONTROL(button4,-1,7,NOTE_ON,35,-1)

# PROGRAM
# labels are words alone on a line followed by a colon
# lables break the program up into chunks of 'code'
# each chunk is executed atomically
# the ordering of items within a chunk is not significant.

init:
    ON(button4) GOTO init           # the rightmost button will clear the looper and start over
    ON(button1) GOTO record         # the leftmost button will start recording the first clip
    CLEAR_ALL                       # THIS IS THE 'ACTION' TAKEN BY THIS CHUNK OF CODE
    
record:
    ON(button1) GOTO play           # the leftmost button will start playing the first clip
    RECORD(1,1)                     # record to sequence 1, layer 1
    
play:
    ON(button1) GOTO stop           # the leftmost button will start stop the sequence
    PLAY(1,1)                       # play sequence 1, layer 1
    
stop:
    ON(button1) GOTO play           # the leftmost button will start playing the clip
    STOP_ALL                        # stop everything

```

The words in UPPERCASE are part of the language.
The words in lowercase are user defined identifiers.



More complicated example
------------------------

At a generic level these scripts can setup the looper as a live
looping tool, equivilant to many hardware loopers.  But going
further, these scripts can allow you to setup complex behaviors
that happen in a specific order, using a minimum of control
movements, so that you can focus on your playing and singing,
rather than on your foot position.

For now we are still assuming a default stereo in, stereo out
configuration for the RECORD and PLAY verbs.

**Touch of Grey**

This script is built specifically to help me perform a live
looped version of the Grateful Dead song "Touch of Grey*.

The song basically looks like this, with some notes

    - INTRO                   very difficult to peg 1
    - VERSE1                  the verse, chorus, and bridge are recorded
    - CHORUS                     the first time through.
    - BRIDGE1
    - VERSE2                  the second time through, I will play bass
    - CHORUS                       while singing
    - BRIDGE2
    - LEAD THRU VERSE         I will play lead over the guitar and bass
    - LEAD THRU CHORUS           through the whole verse, chorus, and bridge
    - LEAD THRU BRIDGE
    - VERSE3                  don't need to play, sing over loop, strum along lightly
    - CHORUS                  strum along lightly
    - CHOURS                  strum along forefully
    - END(INTRO + NOTE)       nearly impossible to pull off
    
The chorus is recorded separately for duplciation at the end.

There are three main problems I have had with this song.

- the INTRO is syncopated and very difficult to record correctly.
  Also, I would typically record only half of it as I need to be
  playing it when I start recording it, so I warm up one time thru.
  
- there is no time to switch from guitar to bass between BRIDGE1 and
  VERSE2 when I would like to start recording the bass

- the end is basically the full INTRO, which is conceptually doable,
  though difficult in practice,
  
- The very end is nearly impossible to pull off. It consists basically
  of a single note.  Maybe there would be some way to automatically
  derive loop points from some other track (i.e. the VERSE), where I
  sustain the note, and/or fade it out.
    
So lets assume that I can record the intro, switch instruments nearly
instantaneously, and invent a verb to find the the first note of a
track and automatically find points in the weveform to loop it to
make a consistent note that fades out.

```

DEFINE_CONTROL(button1,-1,7,NOTE_ON,32,-1)
DEFINE_CONTROL(button2,-1,7,NOTE_ON,33,-1)
DEFINE_CONTROL(button3,-1,7,NOTE_ON,34,-1)
DEFINE_CONTROL(button4,-1,7,NOTE_ON,35,-1)

init:
    ON(button4) GOTO init
    ON(button1) GOTO intro
    CLEAR_ALL                       
    
    # the following are defined as macros that send midi events to the iPad
    
    SYNTH_PATCH(BASS)                       # select the bass patch
    SYNTH_VOLUME(0)                         # turn the synth down
    GUITAR_VOLUME(77)                       # turn the guitar up

# First time through, record the guitar

intro:
    ON(button1) GOTO verse1
    RECORD(1,1)                             # record intro on guitar
verse1:
    ON(button1) GOTO chorus1         
    RECORD(2,1)                             # record verse on guitar
chorus1:
    ON(button1) GOTO bridge1         
    RECORD(3,1)                             # record chorus on guitar
bridge1:
    ON(button1) GOTO verse2
    RECORD(4,1)                             # record bridge on guitar

# second time through, record the bass
# after this, no button presses are needed.
# the song just "goes"

verse2:
    GUITAR_VOLUME(0)                        # turn the guitar down
    SYNTH_VOLUME(77)                        # turn the synth up
    PLAY(2,1,1) GOTO chorus2                # play guitar verse one time, then advance
    RECORD(2,2)                             # record verse on bass
chorus2:
    PLAY(3,1,1) GOTO bridge2                # play guitar chorus one time, then advance
    RECORD(3,2)                             # record chorus on bass
bridge2:
    PLAY(4,1,1) GOTO verse3                 # play guitar bridge one time, then advance
    RECORD(4,1)                             # record bridge on bass

# third time through
# play guitar lead over verse, chorus, and bridge on with both guitar and bass playing

verse3:
    SYNTH_VOLUME(0)                         # turn the synth down
    GUITAR_VOLUME(77)                       # turn the guitar up
    PLAY(2,1,1) GOTO chorus3                # play guitar verse one time, then advance
    PLAY(2,2)                               # play bass verse
chorus3:
    PLAY(3,1,1) GOTO bridge3                # play guitar chorus one time, then advance
    PLAY(3,2)                               # play bass chorus
bridge3:
    PLAY(4,1,1) GOTO verse4                 # play guitar bridge one time, then advance
    PLAY(4,1)                               # play bass bridge

# forth time thru, just sing and sort of strum
# thru verse and two choruses

verse4:
    PLAY(2,1,1) GOTO chorus4                # play guitar verse one time, then advance
    PLAY(2,2)                               # play bass verse (according to guitar track timing)
chorus4:
    PLAY(3,1,2) GOTO ending                 # play guitar verse two times, then advance
    PLAY(3,2)                               # play bass chorus (according to guitar track timing)

# ending
# since we recorded the 1/2 of the intro, we play it twice
# at the end, but we have to add the bass notes here

ending:                                     # one time through the intro to add the bass
    GUITAR_VOLUME(0)                        # turn the guitar down
    SYNTH_VOLUME(77)                        # turn the synth up
    PLAY(1,1,1) GOTO end2
    RECORD(1,2)
end2:
    PLAY(1,1,1) GOTO end3                   # play intro 2nd time for ending
    PLAY(1,2)                               # play intro bass ppart (based on guitar track timing)

# the final ending assumes a lot
# that there is an identifiable loopable "one" within the verse
# and that the volume controls are instantaneous

end3:
    GUITAR_VOLUME(77)                       # turn the guitar up
    PLAY_NOTE(2,1,2.5,0.5) GOTO finish      # find a note at start of verse guitar track, and play it for 2.5 seconds
    PLAY_NOTE(2,2,2.5.0.5)                  # do the same with the 
finish:
    # do nothing for time being
    # so I can see ending state
    # GOTO init

```    

Implementation wise, this will be compiled to a byte code of some sort

Facilitation of the display is not clear to me.   We know the maximum
number of sequences and tracks with this one.  Not sure it makes sense
to have a touch or mouse UI.


Macro Definition
----------------

**standard_macros.script**

Note that CC stands for the number for continuous controller.

```

START_MACRO  GUITAR_VOLUME(n)
    SEND_MIDI(0,11,CC,7,n)         # cable, channel, msg, param1, param2 
END_MACRO    

START_MACRO  GUITAR_VOLUME(n)
    SEND_MIDI(0,7,CC,7,n)
    MACRO    

```

The implementation of SEND_MIDI will require a separate
midi device, I'm thinking a teensy 3.2 or 3.6, that
can interface to the iPad.  I don't see a role for
it to be bi-directional at this time, although there
is no reason it could not be.

The communication between the rPi and that teensy
is essentially midi (or text) to USB midi. Communicating
with that teensy, in my current rPi development environment,
may be complicated by the fact that I rely on the primary
seria port on the rPi for downloading code to the rPi,
and debuggong output.

Not sure I can use the rPi's sketchy 2nd 'miniSerial'
port for that.

Might make a fun starting project.


Other Documents
----------------

There are a variety of other documents associated with this looper:

[Live Looping Issues and Discussion](issues.md)
