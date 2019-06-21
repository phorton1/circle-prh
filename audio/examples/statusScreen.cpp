#include "statusScreen.h"
#include <audio/bcm_pcm.h>
#include <audio/AudioStream.h>


#define USE_STATUS_TASK

#ifdef USE_STATUS_TASK

	#include <circle/sched/scheduler.h>
    
    class AudioStatusUpdateTask : public CTask
    {
        public:
            AudioStatusUpdateTask(statusScreen *pStatusScreen)
                : m_pStatusScreen(pStatusScreen)  {}
            ~AudioStatusUpdateTask() {}
        
            void Run(void)
            {
                while (1)
                {
                    m_pStatusScreen->update();
                    CScheduler::Get()->MsSleep(100);
                }
            }
            
        private:
            statusScreen *m_pStatusScreen;
    };

    AudioStatusUpdateTask *s_pUpdateStatusTask = 0;

#endif




typedef struct
{
    int y;
    int x;
    const char *text;
} staticFormEntry;


#define FORM_FLAG_PROCESSOR_FLOAT   1

typedef struct
{
    int y;
    int x;
    const char *format;
    
    u32 *p_value;
    u32 last_value;
    u8  flags;
    
} formEntry;



staticFormEntry static_form[] =
{
    {  0,  12,  "IN          OUT" },
    {  2,  0,   "Blocks" },
    {  3,  0,   "Other" },
    {  4,  0,   "Wrong" },
    {  5,  0,   "Ov/Und" },
    {  6,  0,   "Diff" },
    
    {  0, 40,   "Library" },
    {  2, 40,   "Overflows" },
    {  3, 40,   "Processor" },
    {  3, 62,   "Max" },
    {  4, 40,   "Memory" },
    {  4, 62,   "Max" },
    

};


formEntry form[] =
{
    {0, 0,  "%-8d",     0, 0, 0},     // main_loop_counter
    {2, 12, "%-8d",     0, 0, 0},     // in_block_count
    {2, 24, "%-8d",     0, 0, 0},     // out_block_count
    {3, 12, "%-8d",     0, 0, 0},     // in_other_count
    {3, 24, "%-8d",     0, 0, 0},     // out_other_count
    {4, 12, "%-8d",     0, 0, 0},     // in_wrong_count
    {4, 24, "%-8d",     0, 0, 0},     // out_wrong_count
    {5, 12, "%-8d",     0, 0, 0},     // overflow_count
    {5, 24, "%-8d",     0, 0, 0},     // underflow_count
    {6, 12, "%-8d",     0, 0, 0},     // diff
                            
    {0, 52,  "%-d",     0, 0, 0},     // audio stream update needed
    {2, 52,  "%-d",     0, 0, 0},     // audio stream overflows
    {3, 52,  "%02.1f",  0, 0, FORM_FLAG_PROCESSOR_FLOAT},     // processor
    {3, 74,  "%02.1f",  0, 0, FORM_FLAG_PROCESSOR_FLOAT},     // processor max
    {4, 52,  "%-d",     0, 0, 0},     // memory
    {4, 74,  "%-d",     0, 0, 0},     // memory max
    
    
};

#define NUM_STATIC_ENTRIES ((int) (sizeof(static_form) / sizeof(staticFormEntry)))
#define NUM_FORM_ENTRIES   ((int) (sizeof(form) / sizeof(formEntry)))


s32 block_diff = 0;
u32 processor_usage = 0;
u32 processor_usage_max = 0;
u32 memory_used = 0;
u32 memory_used_max = 0;


statusScreen::statusScreen(CScreenDevice *pScreen)
{
    screen = pScreen;
    initialized = false;
    
    extern u32 main_loop_counter;
    form[0].p_value = &main_loop_counter;
    form[1].p_value = &bcm_pcm.in_block_count;
    form[2].p_value = &bcm_pcm.out_block_count;
    form[3].p_value = &bcm_pcm.in_other_count;
    form[4].p_value = &bcm_pcm.out_other_count;
    form[5].p_value = &bcm_pcm.in_wrong_count;
    form[6].p_value = &bcm_pcm.out_wrong_count;
    form[7].p_value = &bcm_pcm.overflow_count;
    form[8].p_value = &bcm_pcm.underflow_count;
    form[9].p_value = (u32 *) &block_diff;

    form[10].p_value = (u32 *) &AudioStream::update_needed;
    form[11].p_value = &AudioStream::update_overflow;
    form[12].p_value = &processor_usage;
    form[13].p_value = &processor_usage_max;
    form[14].p_value = &memory_used;
    form[15].p_value = &memory_used_max;
}


void statusScreen::cursor(int x, int y)
{
    print_screen("\x1b[%d;%dH",y+1,x+1);
}



void statusScreen::init()
{
    // clear the screen
    print_screen("\x1b[H\x1b[J");
    // hide the cursor
    print_screen("\x1b[?25l");
    
    for (int i=0; i<NUM_STATIC_ENTRIES; i++)
    {
        staticFormEntry *e = &static_form[i];
        cursor(e->x,e->y);
        print_screen(e->text);
    }

    #if 1    
        int y = 8;
        cursor(0,y++);
        print_screen("Object           CPU       MAX");
        y++;
        int x = 0;
        for (AudioStream *p = AudioStream::first_update; p; p = p->next_update)
        {
            cursor(x,y++);    
            print_screen("%s%d\n",p->dbgName(),p->dbgInstance());
            if (y >= 23)
            {
                y = 10;
                x = 40;
            }
        }
    #endif
    
    // AudioStream::update_overflow = 0;
    // AudioStream::update_needed = 0;
        // clear overflows that occur during startup
    initialized = true;
    
    #ifdef USE_STATUS_TASK
        s_pUpdateStatusTask = new AudioStatusUpdateTask(this);
    #endif
}


void statusScreen::update()
{
    if (!initialized)
    {
        init();
        return;
    }
    
    block_diff = ((s32)bcm_pcm.out_block_count) - ((s32)bcm_pcm.in_block_count);
        // the difference between in and out block counts, should be very close to 0

    // The raw processor numers are millionths of a second (us) spent in update(),
    // INCLUDING any non-audio system interrupts that happen during it.
    // The system is essentially driven by the LRCLK of some i2s device.
    //
    // The raw processor numbers are turned into a floating point number
    // from 0 to 100 based on the amount of time for one full raw audio
    // buffer to be transmitted by i2s.  For example, at 44.1khz, 128 stereo
    // samples, this is about 2.9ms, or 2900us.
    
    float usPerBuffer = 1000000 / bcm_pcm.m_SAMPLE_RATE;
    processor_usage = AudioStream::cpu_cycles_total;            
    processor_usage_max = AudioStream::cpu_cycles_total_max;
    
    
    // move exact memory block counters into u32's
    
    memory_used = AudioStream::memory_used;
    memory_used_max = AudioStream::memory_used_max;
        
    for (int i=0; i<NUM_FORM_ENTRIES; i++)
    {
        formEntry *e = &form[i];
        if (e->last_value != *e->p_value)
        {
            e->last_value = *e->p_value;
            cursor(e->x,e->y);
            if (e->flags & FORM_FLAG_PROCESSOR_FLOAT)
            {
                float value = ((float)e->last_value)/usPerBuffer;
                print_screen(e->format,value);
            }
            else
            {
                print_screen(e->format,e->last_value);
            }
        }
    }

    #if 1
        int x = 0;
        int y = 10;
        for (AudioStream *p = AudioStream::first_update; p; p = p->next_update)
        {
            if (p->cpu_cycles != p->last_cpu_cycles)
            {
                p->last_cpu_cycles = p->cpu_cycles;
                float value = ((float)p->cpu_cycles)/usPerBuffer;
                cursor(x+17,y);
                print_screen("%-02.1f",value);
            }
            CScheduler::Get()->Yield();
            if (p->cpu_cycles_max != p->last_cpu_cycles_max)
            {
                p->last_cpu_cycles_max = p->cpu_cycles_max;
                float value = ((float)p->cpu_cycles_max)/usPerBuffer;
                cursor(x+28,y);
                print_screen("%-02.1f",value);
            }
            y++;
            if (y >= 23)
            {
                y = 10;
                x = 40;
            }
            
        }
    #endif
}


void print_screen(const char *pMessage, ...)
{
    CScreenDevice *screen = (CScreenDevice *) CDeviceNameService::Get()->GetDevice("tty1",false);
    if (screen)
    {
        va_list var;
        va_start(var, pMessage);
        CString Message;
        Message.FormatV(pMessage, var);
        screen->Write((const char *) Message, Message.GetLength());
        va_end(var);
    }
    CScheduler::Get()->Yield();
        // This method, particularly, Yields often to ensure that the audio update
        // task happens expeditiously.
}
