//
// awsVuMeter.h
//
// An object that combines the ws ui and audio systems
// to provide a vu meter to audio applications.

#ifndef _awsVuMeter_h
#define _awsVuMeter_h

#include <audio/Audio.h>
#include <ws/wsWindow.h>

#define OWNER_DRAW

class awsVuMeter : public wsWindow
    // should be sized so that the major dimension plus 1
    // is evenly divisible by num_divs
{
	public:
	
		~awsVuMeter() {}
		
        awsVuMeter(wsWindow *pParent,u16 id, s32 xs, s32 ys, s32 xe, s32 ye,
                   u8 horz, u8 num_divs);

        void setAudioDevice(
            const char *name,               // the audio device name (i.e. "tdmi", "i2si", "record", etc)
            u8         instance,            // the instance of the device (probaly 0)
            u8         channel);            // the output channel on the device to monitor

        
	protected:
	
   		virtual void update();
        virtual void onDraw();
        
    private:
    
        AudioAnalyzePeak *m_pPeak;
        AudioConnection  *m_pConnection;
        
        u8 m_horz;
        u8 m_num_divs;

        u8 m_last_value;
        u8 m_next_value;
        
        u16 m_hold_red;
        bool m_running;
        
};


#endif  // _awsVuMeter_h
