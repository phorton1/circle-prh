
#ifndef AudioDevice_h
#define AudioDevice_h

#include "AudioTypes.h"


class AudioDevice
{
public:

	~AudioDevice() {};
	AudioDevice(
        u16  num_controls,
        audioControl_t *control_setup) :
            m_numControls(num_controls),
            m_pControls(control_setup)
    {
        m_instance = 0;
    }
    
	virtual u16 getType() { return 0; } // = 0;
	// virtual const char *getName() = 0;
    // virtual u16 getInstance() { return m_instance; }
    
	// controls

	u16 getNumControls() 		  	{ return m_numControls; }
	audioControl_t *getControlsl()	{ return m_pControls; }

protected:
	
	// virtual const char *getName() = 0;
    virtual u16 getInstance() { return m_instance; }

    friend class AudioSystem;
    
    virtual void start() {}
    virtual void stop()  {}
    
    u16              m_instance;
	u16              m_numControls;
	audioControl_t   *m_pControls;
    
};



class AudioCodec : public AudioDevice
{
public:
    
    AudioCodec(
            u16 num_controls = 0,
            audioControl_t *control_setup = NULL) :
        AudioDevice(num_controls,control_setup)
    {
        s_pCodec = this;
    }
    virtual u16 getType()  { return AUDIO_DEVICE_CODEC; }
	static AudioCodec *getSystemCodec() { return s_pCodec; }
    
private:
    friend class AudioSystem;
    
    static AudioCodec *s_pCodec;
};


#endif	// !AudioStream_h
