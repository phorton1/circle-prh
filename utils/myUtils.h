#ifndef __myutils_h__
#define __myutils_h__

// This file defines a very limited number of global methods,
// useful for probing and debugging circle itself.

// Semi-official debugging output from Circle should be added
// with CLogger::Get() or a member if available, but there are
// lots of times you just want to insert a printf() or a delay()
// to see what is happening.

// These methods require that there is a deviceNameService,
// that tty1 (the serial port) is registered as a device,
// and that there is a CTimer, so they cannot be used in
// debugging the ctors() of the lowest level stuff like
// interrupts, the CTimer, or the Serial Port itself.
//
// But they CAN be used once these devices are initalized,
// i.e. to debug problems with the USB dwhci which, for me
// is currently not working (and crashes the rPi with no
// Logger output) on the rPi3+ (it works on the zero)

#define LOG(f,...)           CLogger::Get()->Write(log_name,LogNotice,f,__VA_ARGS__)
#define LOG_DEBUG(f,...)     CLogger::Get()->Write(log_name,LogDebug,f,__VA_ARGS__)
#define LOG_WARNING(f,...)   CLogger::Get()->Write(log_name,LogWarning,f,__VA_ARGS__)
#define LOG_ERROR(f,...)     CLogger::Get()->Write(log_name,LogError,f,__VA_ARGS__)
#define LOG_PANIC(f,...)     CLogger::Get()->Write(log_name,LogPanic,f,__VA_ARGS__)

extern void delay(unsigned int ms);
extern void printf(const char *f, ...);
extern void display_bytes(const char *s, const unsigned char *p, int len);

// #define DPROBE

#ifdef DPROBE
    #define dprobe(l,f,...)  do_dprobe(l,f,__VA_ARGS__)
    extern void do_dprobe(int l, const char *f, ...);
#else
    #define dprobe(l,f,...)
#endif


#endif // !__myutils_h__