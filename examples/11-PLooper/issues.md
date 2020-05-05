[back to reamde](readme.md)

Live Looping Issues
===================

The "need time to change instruments" problem
---------------------------------------------

One issue with all single player looping is that you must
first play something before you can loop it and record a
subsequent track over it, and when recording multiple layers
there is usually some time needed to switch instruments.

For example, if I want to record a rythmn guitar part, and
then add bass to it, it is virtually impossible to switch
from the guitar to the bass, and be ready to start playing
the bass, in a single beat.  So typically, you would first
record the guitar part, then play it back one time, perhaps
singing a second verse, while you change the instrument,
i.e. turn down the acoustic guitar and turn up the synth,
and get ready to start playing bass, perhaps leading into
the third time through.

Then on the third time through, you would record the bass,
and then let it also play while you sang another verse,
or did a lead with it.

Some dead time is needed to switch instruments.


The "Backing Out Problem
------------------------

Another issue with this kind of live looping is that
it can sound really slim if you build up a number of layers,
and then have to switch to a single live instrument (guitar),
say if there is a bridge, or for the end of the song,
which is typically different than the rest of the song.

It reminds us that there is a "lowest common denominator"
to these songs, the solo acoustic guitar (with possible
simultaneous synth).

Even allowing for the problem, it can be very difficult to
get back to the correct sound (patch settings), volume levels,
and be emotionally and physically prepared to start playing
solo.  Especially if you feel the need (often) to be playing
already at that moment, so you are actually adding to the
problem by compounding the acoustic guitar in the final moements
before the transition.

It also tends to make all the songs have the same basic
emotional content, of an acoustic guitar by itself at
the beginning, and somewhere else in the song.

Almost makes me want to just go with a full pre-sequenced
recording or synchronous midi track in the background.
Drums? .... hmmm.


The "Ending" or "Tempo Change" problem
---------------------------------------

Endings typically want to repeat some small subsection of
the chorus a few times, slow down, and hit a big last
single note.  These are all virtually impossible, or difficult,
to do with existing loopers. OK, you can do the "subsection
of the chorus" thing by setting up loop markers (recording the
chorus as subsequences), and triggering them somehow, but the slowing
down and single last note are really effing hard to solve.

These problems are generally difficult because it is algorihmically
very difficult to slow down, or hold a note, as it were, within
an audio track.

However, given the notion that the only true audio in my system
is the guitar (with or without effects), and that all the other
"instruments" in my system are midi synths, then concievably,
the slowing down and stopping issue could be mitigated by
recording not the audio of the synthesizer, but rather the
midi sequence, and looping that, instead of, or in addition
to, the audio from the guiar.

That midi sequence could be replayed in sync with audio
tracks that are looped, or it could be fed back to the
synthesizer as midi events with a slow down, to 0, for the
last note (hold indefinitely).