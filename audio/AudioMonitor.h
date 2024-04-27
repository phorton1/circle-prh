// AudioMonitor.h
//
// A debugging class that uses the Circle HDMI Screen device
// and ansi escape codes to show statistics from the AudioSystem
// and bcm_pcm.  Client constructs in context of a CScreenDevice,
// which MUST be the HDMI device, and then periodically calls
// Update() ... at a UI frame rate 30/frames a sec or so.

#ifndef AudioMonitor_h
#define AudioMonitor_h

class AudioMonitor
{
public:

	AudioMonitor() {};
	~AudioMonitor() {}

	void Update();

};


#endif	// !AudioMonitor_h
