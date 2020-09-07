# Looper Concepts

## Caveat

After struggling mightily with USB mice, and MIDI devices, I had to give up on them,
at least for the time being.  It seemed that both were so processor intensive as to
interfere with the Audio processing, so I have reverted to serial command input
which, so far, seems to work.  I need to focus on the state machine.

## Basic Architecture - tracks, clips, (current, used, and selected)

The looper consists of 4 sequential "tracks" with upto 3 layered "clips" per track.

The only valid command for a completely empty, initialized looper, is to start
recording (RECORD) clip0 on track0. I will also refer to this as clip(0,0).
The track that is currently recording or playing is considered the "current"
track.

There is the notion of a "used" track or clip.   A used clip is one that
has been recorded or IS BEING recorded.  A used track is a track that has a
used clip. So, when the looper is first started there are no used tracks or
clips, but as soon as one starts to record clip(0,0), there is one used
track, and it has one used clip at that moment.

There is a notion of a "selected" track and clip. The user may "select"
a given track and/or clip via the user interface NEXT_CLIP and NEXT_TRACK
buttons.  However, they cannot just select ANY clip in the matrix.  They
are constrained to selecting only used tracks and clips, or the NEXT EMPTY
clip or track.  And of course, they can only select upto 4 tracks of 3
clips each.

Thus, when the looper is empty, a user is constrained to selecting clip(0,0),
and the NEXT_CLIP and NEXT_TRACK buttons are disabled (or not visible).
But as soon as one starts to record clip(0,0), then both buttons are enabled
(or become visible) and the user can select any of the following track/clips:

    clip(0,0) - the current (first) clip being recorded
    clip(0,1) - the next empty clip in the current track
    clip(1,0) - the 0th, empty clip in the next available track

Note that the selected track and clip can be different than the
currently playing/recording track or clip.

## COMMANDS and UI

The NEXT_CLIP and NEXT_TRACK commands change the selected track and clip.

Most other COMMANDS can be generally thought of as acting on the selected
track and clip, or on the looper in general.  COMMANDS not only affect the
selected track and clip, but they may (likely) affect the currently
playing or recording track/clip as well.

Recorded clips in the UI show up as green bars.  Clips being recorded
in the UI show up as red bars.

The user interface shows a thin white box around the currently selected track,
and a bolder yellow or red box around the selected clip, depending on whether it has
content, or not.  It also flashes those bold boxes (or a white box around the
given clip), when a command is "pending", which will be described later.

The UI generally follows, and needs to follow the state machine.

In the simple case of the initial RECORD command, the command clearly
applies to clip(0,0) and tells the looper to start recording it.


## BASE CLIP, LOOP POINTS, MULTIPLES, and PENDING COMMANDS

So, now we introduce the notion of the BASE CLIP in a track, which is
just the first (0th) clip in the track.  The 0th clip in a track is
always recorded first, and is called the "base clip" because the length
of it determines the length of all the other clips in the track, which
must be integral multiples of the base clip length (1z, 2x, 3x, etc).

For example, let's say the user starts recording clip(0,0)
and 15 seconds later decides to start looping it and recording clip(0,1)
on top of it.   The user will press the following buttons:

- **RECORD** - starts recording (default selected) clip(0,0)
    a red bar moves from left to right on the screen indicating
    "progress" of the recording.

- **SELECT_NEXT_CLIP** - highlighs clipt(0,1) the next clip in track(0)
    in red indicating it will record on the next RECORD command

- 15 seconds goes by ... and the user presses ...

- **RECORD** - which starts IMMEDIATELY looping (playing) clip(0,0)
   and starts IMMEDIATELY recording clip(0,1).

   *If the user did not want to start recording another layer,
   but did want to start looping clip(0,0) they would have pressed PLAY.
   Or if they wanted to stop recording clip(0,0) and save it for
   later playback, they would have pressed STOP. But they pressed
   "RECORD", so clip(0,1) starts recording as clip(0,0) starts playing.*

Now, lets say the user plays a lead for 10 seconds, and then
hits the PLAY button (clip(0,1) is still selected.)

There are still 5 seconds of clip(0,0) to play!

At that moment, the PLAY command is considered to be **PENDING**,
and it won't actually take place until the base clip has completed
(comes around to a **LOOP POINT**).

## Integral Multiples of Base Track ..

So, in the above example the looper keeps playing clip(0,0) and recording
clip(0,1) for 5 more seconds AFTER the **PLAY** button is pressed,
until the **BASE CLIP** comes around to a **LOOP POINT** and at that time
(5 seconds later) the looper **saves** the recording for clip(0,1) and starts
**playing it synchronously** with clip(0,0).  Notice that in this case both
clips have exactly the same length (15 seconds).

Alternatively, if the user had instead waited 22 seconds (instead of
10 seconds) in the example above, by that point in time the looper would
have been 7 seconds into the **SECOND** time through clip(0,0)
and there would still be 8 (EIGHT) seconds of clip(0,0)
to play! So clip(0,1) would end up being exactly twice as
long as clip(0,1).

This is how it works to ensure that the length of clips(1..3)
are always an integral multiple of the length of clip(0) in a
given track.

More subtely, in this section we introduced the notion that
commands that affect the BASE CLIP recording *USUALLY* happen
immediately, but that commands that happen while a clip is
*PLAYING* usually are considered PENDING until the next
LOOP POINT.


## STATE MACHINE 1 - Changing commands in the middle of the stream

Because of pending commands, it is possible to "change your mind"
about certain commands, as long as you press the buttons in time.

Obvioulsy(?) we are not talking about the "immediate" commands that
can happen when recording clip(0) in a given track, but rather for
commands that have been issued (are PENDING) but have not yet taken
effect.

The basic rule is that CHANGING THE SELECTED TRACK or CLIP
cancels any command that may be PENDING.

The user will record 15 seconds on clip(0,0), then start PLAYING it,
THEN decide to start recording clip(0,1), but then change their mind
and decide to cancel the upcoming recording (and just continue
playing clip(0,0)).   So, from a completely empty looper, they issue
the following sequence of commands

- **RECORD** - start recording clip(0,0)
- play guitar for 15 seconds
- **PLAY** - they press "play" and clip(0,0) starts looping
- **SELECT_NEXT_CLIP** - the user selects clip(0,1). It is highlighted in red.

Now, while clip(0,0) is loopig through it's 15 seconds of fame
over and over again, at some point, lets say 5 seconds into
any one of those loops, the user presses the RECORD button.

- **RECORD** - pressed 5 seconds into the 15 second base clip

So there are still 10 seconds for the base clip(0,0) to play
before the pending RECORD command will take effect. If the user
does nothing more at this point, in 10 seconds, it **WILL** start
recording (on a LOOP POINT of the BASE CLIP).

But they change their mind, and decide to go back to just
*playing* the base clip.  So they rapidly hit the SELECT_NEXT_CLIP
button.

- **SELECT_NEXT_CLIP** - the user selects goes back to selecting
   the base clip(0,0). It is highlighted in yellow.

This has the effect of clearing the pending RECORD command,
so the looper just keeps doing what it was doing, which is
playing back clip(0,0) over and over again.

To summarize the points of this section:

- **CHANGING THE SELECTION DELETES ANY PENDING COMMAND!!**

- **THE PENDING COMMAND ACTS ON THE SELECTED TRACK OR CLIP AT THE NEXT LOOP POINT**

- **commands effected while BASE CLIP0 is recording are immediate**


The point of this whole discussion was to note that there is
only ONE PENDING COMMAND at any given time, and it pertains
to the CURRENTLY SELECTED TRACK.

Although it seems trivial, this will become important when we
discuss the cross-fade mechanism.


# CROSSFADES

I had a working implementation of the above basic state machine until
I noticed the need for "cross fades".  Cross fades have made the whole
thing MUCH more complicated.

The issue is that you cannot just start and stop playing recorded waveforms
instantaneously with good results.   In practice, if you start playing back
and looping a raw recording, what ends up happening are very noticable "clicks"
in the playback.

If the recordings are very quiet, or silent, at the beginning and ends, then
there are no clicks, but if you are recording a continuous sound that crosses
that boundry, then, the waveforms at the end don't match up back to the beginning,
resulting in those artifacts.

The buffers, which contains 128 samples, which are numbers from -32K to +32K,
when taken as a whole, describe the waveform to be reproduced digitally,
and usually, they will start in the middle of a sound (with a nonzero value)
and end in the middle of another sound (with another nonzero value). So,
instead of a smooth wave being produced by the transition, you (statistically
very likely) get an abprupt "spike" or change in the waveform, and the resulting
click.

BTW, FWiW, at 44.1Khz (CD quality sound) there are about 350 (stereo) buffers processed
per second per clip, so there's lot of processing going on.

So, what is needed, what I implemented, is that when the sound loops back into
itself, we "cross fade" it ... that is, we "fade out" the first instance of the loop
as we "fade in" the 2nd instance.  This "fading", by the way, is simply a mathematical
linear process on the samples as we render them.

But orchestrating it is complicated.

Although the cross fade I implemented seems to work best at about 100 milliseconds
(1/10th of a second), for illustration, to show why I took this approach, lets say
that I want one full second of fading in and fading out (overlap) in a given loop.
Let us further say that the clip is EXACTLY 15 seconds in duration from the user's
perspective.  I will try to illustrate:


    0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
    [this is the duration of the sound the user expects to loop ] ^^^
                                                                   |
    this is an extra second of sound  ---------------------------- +
    I must record to make that happen

If I (the looper) only recorded 15 seconds of sound for the user's 15 second
loop, and if I (the looper) want to fade out and in for one second, I would
**not have the samples to do it!** ... If I only recorded 15 seconds, I would
need to artificially "cut" the loop time to 14 seconds!!

That's why I chose one full second of cross fade for my example.
Clearly a loop that was off by one full second from when you pressed the
buttons would be unusable.   The user more or less expects the loop duration
to be exactly the duration between their button presses, not some algorithmic
function of it!  Especially not off by a full second!

So, what I (the looper does) is when you are recording clip(0,0) and press the PLAY
button at exactly 15 seconds, we DO start playing clip(0,0) from the begining
(WITH a one second fade in), BUT we CONTINUE recording "off the end of it"
for another full second ...

Then the second time we get to the end of the 15 seconds, we play that extra second,
but fading it out this time, as we simultaneously start playing the next time through
from the beginning, fading it in.

So, all clips are recorded a little longer than the button presses (for the base
clip) or LOOP POINT (for subsequent clips).

In the implementation, the cross fade time is represented as a number of
CROSSFADE_BUFFERS to keep, where, once again, at 44.1Khz, there are about
350 buffers per second, so in this illustation CROSSFADE_BUFFERS would be
about 350 buffers (1 second), but the actual working value in the code is more
like 30 buffers(about 1/10 of a second).

My initial proof of concept implementation showed that this eliminated
the clicks quite nicely from a continous human voice and will likely
be "good enough" for my purposes.


## Crossfade State Machine is Complex

This added a tremendous amount of complexity to the state machine.

Whereas in the "simple" state machine, a "clip" only needed one "pointer"
into it's buffers (the current buffer pointer for playback OR recording),
now the system must maintain multiple pointers into the buffer, as a
single clip may be simultaneously playing AND recording, or playing
in TWO different places, at the same time.

And in fact, two completely separate tracks may be playing and/or
recording at the same time!

This also affects, and is affected by "pending" commands, and goes
to the above discussion about "cancelling" a pending command. In the
previous "simple" state machine, I could get away with just replacing
the "pending command" with a new one (or deleting the old one) as there
was no interaction between "previous" and "next" clips recording or
playback.


## Crossfade state machine - previous, current, and selected track/clips.

I *believe* that the state machine can still work with simple "pending" commands,
that happen at the LOOP_POINT of a given BASE_CLIP, and that with a relatively
short (1/10 of a second) CROSSFADE there is little to no chance that a command
will be issued during that period of time that will be non-sensical to the user.

But it *may* require double buffering of the pending commands. Not sure.

Maybe not.  As the "cross fade" period of time indicates that SOME OTHER
TRACK (or no TRACK) has become the next target for commands.

Maybe not.
