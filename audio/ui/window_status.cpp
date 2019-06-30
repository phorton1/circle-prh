
#include "window_status.h"
#include <circle/sched/scheduler.h>
#include <circle/logger.h>
#include <audio/Audio.h>

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
            AudioSystem::resetStats();
            bcm_pcm.resetStats();
        }
    }
    
    if (m_pTitlebar->Callback(pMsg))
    {
        return;
    }
}



//-----------------------------------------------------
// form display routines
//-----------------------------------------------------

#define FONT_WIDTH      8
#define FONT_HEIGHT     12
#define FONT_NAME       FONT_8X12

#define FT_STATIC_TEXT   0x0001
#define FT_U32_PTR       0x0010
#define FT_S32_PTR       0x0020
#define FT_U32_FXN       0x0100
#define FT_CPU_FXN       0x0200
#define FTS_CPU          0x1000
#define FTS_CPU_MAX      0x2000

typedef struct formEntryStruct
{
    u16 x;
    u16 y;
    u32 type;
    void *fxn_ptr;
    const char *format;
    formEntryStruct *next;
    u32 last_value;
}   formEntry;


formEntry *firstFormEntry = 0;
formEntry *lastFormEntry = 0;
s32 block_diff = 0;

extern u32 main_loop_counter;



void addFormEntry(u16 y, u16 x, u32 type, const char *format, void *fxn_ptr=0)
{
    formEntry *entry = new formEntry;
    memset(entry,0,sizeof(formEntry));
    
    entry->x = x;
    entry->y = y;
    entry->type = type;
    entry->format = format;
    entry->fxn_ptr = fxn_ptr;
    
    if (lastFormEntry)
        lastFormEntry->next = entry;
    else
        firstFormEntry = entry;
    lastFormEntry = entry;
}


void CStatusWindow::show(s16 x, s16 y, const char *format, ...)
{
    va_list var;
    va_start(var, format);
    CString str;
    str.FormatV(format, var);
    va_end(var);
    const char *cchar = (const char *) str;
    s16 useX = STATUS_MARGIN*2 + (x * FONT_WIDTH);
    s16 useY = STATUS_MARGIN + APP_TOP_MARGIN + (y * FONT_HEIGHT);
    UG_PutString(useX,useY,(char *)cchar);
}


void CStatusWindow::init()
{
    addFormEntry(0, 15, FT_STATIC_TEXT,   "IN         OUT" );
    addFormEntry(2, 0,  FT_STATIC_TEXT,   "Blocks" );
    addFormEntry(3, 0,  FT_STATIC_TEXT,   "Other" );
    addFormEntry(4, 0,  FT_STATIC_TEXT,   "Wrong" );
    addFormEntry(5, 0,  FT_STATIC_TEXT,   "Ov/Und" );
    addFormEntry(6, 0,  FT_STATIC_TEXT,   "Diff" );
    addFormEntry(0, 40, FT_STATIC_TEXT,   "Library" );
    addFormEntry(2, 40, FT_STATIC_TEXT,   "Overflows" );
    addFormEntry(3, 40, FT_STATIC_TEXT,   "Processor" );
    addFormEntry(3, 62, FT_STATIC_TEXT,   "Max" );
    addFormEntry(4, 40, FT_STATIC_TEXT,   "Memory" );
    addFormEntry(4, 62, FT_STATIC_TEXT,   "Max" );
    
    addFormEntry(0, 0,  FT_U32_PTR, "%-10d",&main_loop_counter);
    addFormEntry(2, 15, FT_U32_PTR, "%-8d", &bcm_pcm.in_block_count);
    addFormEntry(2, 27, FT_U32_PTR, "%-8d", &bcm_pcm.out_block_count);
    addFormEntry(3, 16, FT_U32_PTR, "%-8d", &bcm_pcm.in_other_count);
    addFormEntry(3, 27, FT_U32_PTR, "%-8d", &bcm_pcm.out_other_count);
    addFormEntry(4, 15, FT_U32_PTR, "%-8d", &bcm_pcm.in_wrong_count);
    addFormEntry(4, 27, FT_U32_PTR, "%-8d", &bcm_pcm.out_wrong_count);
    addFormEntry(5, 15, FT_U32_PTR, "%-8d", &bcm_pcm.overflow_count);
    addFormEntry(5, 27, FT_U32_PTR, "%-8d", &bcm_pcm.underflow_count);
    
    addFormEntry(6, 15, FT_S32_PTR, "%-8d",    &block_diff);
    addFormEntry(0, 52, FT_U32_PTR, "%-8d",    &AudioSystem::s_nInUpdate);
    addFormEntry(2, 52, FT_U32_PTR, "%-8d",    &AudioSystem::s_numOverflows);
    addFormEntry(3, 52, FT_CPU_FXN, "%03.2f",  (void *)&AudioSystem::getCPUCycles);
    addFormEntry(3, 74, FT_CPU_FXN, "%03.2f",  (void *)&AudioSystem::getCPUCyclesMax);
    addFormEntry(4, 52, FT_U32_FXN, "%-8d",    (void *)&AudioSystem::getMemoryBlocksUsed);
    addFormEntry(4, 74, FT_U32_FXN, "%-8d",    (void *)&AudioSystem::getMemoryBlocksUsedMax);
    
    int y = 8;
    addFormEntry(y++,0,FT_STATIC_TEXT,"Object       CPU        MAX");
    y++;
    
    int x = 0;
    for (AudioStream *p = AudioSystem::getFirstStream(); p; p = p->getNextStream())
    {
        // printf("adding %s%d\n",p->getName(),p->getInstance());
        CString *name = new CString();
        name->Format("%s%d",p->getName(),p->getInstance());
        
        addFormEntry(y, x,    FT_STATIC_TEXT, (const char *) *name);
        addFormEntry(y, x+15, FTS_CPU, "%03.2f", p);
        addFormEntry(y, x+27, FTS_CPU_MAX, "%03.2f", p);
        
        y++;
        if (y > 22)
        {
            y = 10;
            x += 40;
        }
    }
    
    AudioSystem::resetStats();
}


void CStatusWindow::draw()
{
    UG_FontSelect(&FONT_NAME);
    UG_SetForecolor(C_CYAN);

    if (!m_bStarted)
        init();
        
    block_diff = ((s32)bcm_pcm.out_block_count) - ((s32)bcm_pcm.in_block_count);
        // the difference between in and out block counts, should be very close to 0
    float usPerBuffer = 1000000 / bcm_pcm.getSampleRate();
        // microseconds that represent full CPU usage    

    for (formEntry *f=firstFormEntry; f; f=f->next)
    {
        if (!m_bStarted || (f->type != FT_STATIC_TEXT))
        {
            u32 value;
            u32 (*fxn)();
            switch (f->type)
            {
                case FT_STATIC_TEXT :
                    show(f->x,f->y,f->format);
                    break;

                case FT_U32_PTR :
                    value = *(u32 *) f->fxn_ptr;
                    if (value != f->last_value)
                        show(f->x,f->y,f->format,value);
                    break;
                
                case FT_S32_PTR :     
                    memcpy(&value,f->fxn_ptr,sizeof(s32));
                    if (*((u32 *)&value) != *((u32 *)&f->last_value))
                        show(f->x,f->y,f->format,*(s32 *) &value);
                    break;
                
                case FT_U32_FXN :
                    fxn = (u32 (*)()) f->fxn_ptr; 
                    value = fxn();
                    if (value != f->last_value)
                        show(f->x,f->y,f->format,value);
                    break;
                    
                case FT_CPU_FXN :
                    fxn = (u32 (*)()) f->fxn_ptr; 
                    value = fxn();
                    if (value != f->last_value)
                    {
                        float us = ((float)value) / usPerBuffer;
                        show(f->x,f->y,f->format,us);
                    }
                    break;

                case FTS_CPU :
                    value = ((AudioStream *) f->fxn_ptr)->getCPUCycles();
                    if (value != f->last_value)
                    {
                        float us = ((float)value) / usPerBuffer;
                        show(f->x,f->y,f->format,us);
                    }
                    break;
                
                case FTS_CPU_MAX :
                    value = ((AudioStream *) f->fxn_ptr)->getCPUCyclesMax();
                    if (value != f->last_value)
                    {
                        float us = ((float)value) / usPerBuffer;
                        show(f->x,f->y,f->format,us);
                    }
                    break;
                
            }   // switch
        }   // if !static text || !started
    }   // for each form entry
    
    m_bStarted = true;
}



