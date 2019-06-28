
#include "window_status.h"
#include <circle/sched/scheduler.h>
#include <circle/logger.h>

#define log_name "win_status"


// After hooking this up for the first time I have a solid indication
// that the mouse is messing with the audio ... looking into prioritizing tasks.


CStatusWindow::~CStatusWindow(void) {}

#define ID_CONTENT      1
#define ID_RESET_STATS  2

#define WINDOW_MARGIN   2
#define STATUS_MARGIN   10



CStatusWindow::CStatusWindow(CApplication *app) :
    CWindow(0,0,UG_GetXDim()-1,UG_GetYDim()-1,0),
    m_pApp(app),
    m_bStarted(0)
{
    // LOG("ctor",0);
    
    m_pTitlebar = new CTitlebar(this,m_pApp,1);
    CTextbox *box = new CTextbox(
        this,
        ID_CONTENT,
        WINDOW_MARGIN,
        APP_TOP_MARGIN,
        UG_GetXDim()-WINDOW_MARGIN,
        UG_GetYDim()-WINDOW_MARGIN,"");
    box->SetBackColor(C_BLACK);
    box->SetForeColor(C_WHITE);
    
    CButton *pb = new CButton(
        this,
        ID_RESET_STATS,
        UG_GetXDim()-100,
        APP_TOP_MARGIN + STATUS_MARGIN,
        UG_GetXDim()-20,
        APP_TOP_MARGIN + STATUS_MARGIN + 20,
        "reset",
        BTN_STYLE_3D);
    pb->SetFont(&FONT_8X12);
        
    // LOG("ctor finished",0);
}


    
void CStatusWindow::Callback(UG_MESSAGE *pMsg)
{
	assert(pMsg != 0);
    if (pMsg->type  == MSG_TYPE_WINDOW)
    {
        if (pMsg->event == WIN_EVENT_UI_FRAME &&
            getUGWindow()->state & WND_STATE_VISIBLE)
        {
            draw();
        }
        else if (pMsg->event == WIN_EVENT_ACTIVATE)
        {
            m_bStarted = 0;
        }
    }
	else if (pMsg->type  == MSG_TYPE_OBJECT && 
             pMsg->id    == OBJ_TYPE_BUTTON && 
             pMsg->event == OBJ_EVENT_PRESSED)
    {
        if (pMsg->sub_id == ID_RESET_STATS)
        {
            AudioStream::resetAllStats();
            bcm_pcm.resetStats();
        }
    }
    
    // weird - if this call is before the above draw(), it seems
    // to write over the status text, but right justified in just
    // exactly the correct location, as if draw() is somehow setting
    // the status textbox text through a re-entrancy issue, maybe
    // with Format() or VA_ARGS.  Yet, if I move the call here,
    // it works, and otherwise, I cannot detect what's wrong.
    // There may be a bug in the pointers in the status window,
    // but it's not important.  It will be entirely re-written.
    
    if (m_pTitlebar->Callback(pMsg))
    {
        return;
    }
}








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


#define FONT_WIDTH      8
#define FONT_HEIGHT  12
#define FONT_NAME    FONT_8X12


void CStatusWindow::show(s16 x, s16 y, const char *pMessage, ...)
{
    //m_pApp->m_SpinLock.Acquire();
    va_list var;
    va_start(var, pMessage);
    CString str;
    str.FormatV(pMessage, var);
    va_end(var);
    const char *cchar = (const char *) str;
    s16 useX = STATUS_MARGIN*2 + (x * FONT_WIDTH);
    s16 useY = STATUS_MARGIN + APP_TOP_MARGIN + (y * FONT_HEIGHT);
    UG_PutString(useX,useY,(char *)cchar);
    //m_pApp->m_SpinLock.Release();
}


void CStatusWindow::init()
{
    printf("status init()\n");
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
    
    for (int i=0; i<NUM_STATIC_ENTRIES; i++)
    {
        staticFormEntry *e = &static_form[i];
        show(e->x,e->y,e->text);
    }

    int y = 8;
    show(0,y++,"Object         CPU       MAX");
    y++;
    int x = 0;
    for (AudioStream *p = AudioStream::first_update; p; p = p->next_update)
    {
        show(x,y++,"%s%d\n",p->dbgName(),p->dbgInstance());
        if (y >= 23)
        {
            y = 10;
            x = 40;
        }
    }

    AudioStream::update_overflow = 0;
    AudioStream::update_needed = 0;
        // clear overflows that occur during startup    
    m_bStarted = true;
    
}


void CStatusWindow::draw()
{
    UG_FontSelect(&FONT_NAME);
    UG_SetForecolor(C_CYAN);

    if (!m_bStarted)
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
    
    float usPerBuffer = 1000000 / bcm_pcm.getSampleRate();
    processor_usage = AudioStream::cpu_cycles_total;            
    processor_usage_max = AudioStream::cpu_cycles_total_max;
    
    
    // move exact memory block counters into u32's
    
    memory_used = AudioStream::memory_used;
    memory_used_max = AudioStream::memory_used_max;
        
    for (int i=0; i<NUM_FORM_ENTRIES; i++)
    {
        // CScheduler::Get()->Yield();
        formEntry *e = &form[i];
        if (1 || e->last_value != *e->p_value)
        {
            e->last_value = *e->p_value;
            if (e->flags & FORM_FLAG_PROCESSOR_FLOAT)
            {
                float value = ((float)e->last_value)/usPerBuffer;
                show(e->x,e->y,e->format,value);
            }
            else
            {
                show(e->x,e->y,e->format,e->last_value);
            }
        }
    }

    s16 x = 0;
    s16 y = 10;
    for (AudioStream *p = AudioStream::first_update; p; p = p->next_update)
    {
        // CScheduler::Get()->Yield();
        if (1 || p->cpu_cycles != p->last_cpu_cycles)
        {
            p->last_cpu_cycles = p->cpu_cycles;
            float value = ((float)p->cpu_cycles)/usPerBuffer;
            show(x+17,y,"%-02.1f",value);
        }
        if (1 || p->cpu_cycles_max != p->last_cpu_cycles_max)
        {
            p->last_cpu_cycles_max = p->cpu_cycles_max;
            float value = ((float)p->cpu_cycles_max)/usPerBuffer;
            show(x+28,y,"%-02.1f",value);
        }
        y++;
        if (y >= 23)
        {
            y = 10;
            x = 40;
        }
        
    }
}


