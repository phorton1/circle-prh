/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef AudioControl_h_
#define AudioControl_h_

#include <stdint.h>

// Paul:
// A base class for all Codecs, DACs and ADCs, so at least the
// most basic functionality is consistent.

// prh:
//
// Given the arduino static object delcaration scheme, and the fact
// that we have to initialize the circle kernel before we can really
// do anything, the call to AudioControl::enable() may be the most
// consistent place to coordinate all the startup code.
//
// Derived teensy AudioStreams typically have a begin() method that they
// called from their constructors.  For Circle we removed the calls from
// the ctorrs, and made begin() and offical AudioStream base class method,
// and call the begin() methods for all AudioStreams() in setup() after
// the kernel is initialized.
//
// Initially we hooked into AudioStream::AudioMemory(), which is a macro
// which calls AudioStream::initialize_memory(), which then just called the
// begin() methods for all stream objects in the order they were statically
// declared (the order they were added to the base "update list").
//
// It turns out the interaction between the codecs and streams must be
// more rigorously defined, the exmaple being that the audio injector
// OCTO clocks must be started in precision synhcrnonization with the
// bcm DMA in order to consistently get the channel interleaving correct.
//
// At that point I leave the existing AudioControlCS42448 object and
// AudioInputTDM/AudioOutputTDM objects as untested "generic" backup
// objects. The TDM object has a different interleaving than the OCTO,
// and the CS42448 *might* work as is.
//
// I will now create an AudioInjectorOctoControl device that works
// with AudioInjectorOctoInput and Output specific devices to deal with
// the non-standard frames and the fpga within the context of the existing
// bcm_pcm code, somehow.




// I'm still not sure.
//
// The "enable" API seems to be weakly linked to the i2s devices.
//
// What if there was more than one audio device.  The bcm only can
// do one i2s stream, but there is also (at least) USB and HDMI and
// possibly PWM output, to consider.  Also note the limitations of the
// teensy fixed s16/44100 API versuse the capabilities and restriction
// of other devices (a seperate issue).
//
// It is as if the static declarations, and their order (and any
// methods that can be called on them early in setup()), constitute
// a kind of configuration script telling the objects that need to
// be initialized and their initil (static) states, and the system
// needs to figure out how to start everything in the correct order.
//
// The AudioMemory() macro itself may not be optimal, either, for
// the kind of scaled up applications I forsee for this library.
//
// But at the heart of it all is the fact that a complicated set of
// things must happen in the right order.  For instance, we already
// init and// "start" bcm_pcm DMA just once for a pair of two i2s
// input and output streams ... we know they are related and set
// them both up in a single call.
//
// I2s is not so fussy about intialization, we can init the wm8731
// before, or after we start the bcm i2s and the channels still come
// out right.
//
// But the OCTO fpga clock must be started at precisely the correct
// phase with the bcm DMA to align the channels, which throws in
// another level of interaction, not just between related streams,
// but between streams and control devices (codecs).
//
// And AudioControl.enable() is the only remaining consistent call
// within the exsting arduino-like API (the only guaranteed call from
// setup(), after the kernel is initialized).
//
// Linux handles this by defininng a device as consisting of the
// two separate (cpu==bcm and codec) device interfaces that it must
// know how to initialize correctly ...


#define AUDIO_INPUT_LINEIN  0
#define AUDIO_INPUT_MIC     1

class AudioControl
{
public:
	virtual bool enable(void) = 0;
	virtual bool disable(void) = 0;
    
	virtual bool volume(float volume) = 0;      // volume 0.0 to 1.0
	virtual bool inputLevel(float volume) = 0;  // volume 0.0 to 1.0
	virtual bool inputSelect(int n) = 0;
};

#endif
