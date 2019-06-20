#include "statusScreen.h"
#include <audio/bcm_pcm.h>
#include <audio/AudioStream.h>

// Status     CPU     usage: NN%    max: NN%
//   Memory   blocks  usage: NNNN   max: NNNN
//
// bcm_pcm
//   blocks:   In: NNNNNNNN  Out: NNNNNNNN   Dif: NNNN
//
// Device      Channels
//             0         1         2         3         4         5         6         7
//   tdmIn     NNNNNNNN  NNNNNNNN  NNNNNNNN  NNNNNNNN  NNNNNNNN  NNNNNNNN  NNNNNNNN  NNNNNNNN
//   mixer     NNNNNNNN  0         0         0
//   mixer     NNNNNNNN  0         0         0
//   tdmOut    NNNNNNNN  NNNNNNNN  0         0         0         0         0         0       
//   
//                         sent      recd
//   tdmIn:0 -> mixer0:0   NNNNNNNN  NNNNNNNN
//   tdmIn:1 -> mixer1:0   NNNNNNNN  NNNNNNNN
//   mixer0 -> tdmOut1     NNNNNNNN  NNNNNNNN
//   mixer1 -> tdmOut2     NNNNNNNN  NNNNNNNN

// How to show it as a connected diagram lol



typedef struct
{
    int y;
    int x;
    const char *text;
} staticFormEntry;


typedef struct
{
    int y;
    int x;
    const char *format;
    
    u32 *p_value; 
    u32 last_value;
    
} formEntry;



staticFormEntry static_form[] =
{
    {  0,  0,   "Status" },
    {  1,  0,   "Blocks  In:" },
    {  1, 23,   "Out:" },
    {  1, 40,   "Diff:" },
    {  1, 60,   "Wrong:" },
    {  2,  6,   "Over:" },
    {  2, 21,   "Under:" },
    
};


formEntry form[] =
{
    {1, 12, "%-8d",  0, 0},     // audioDevice.in_block_count
    {1, 28, "%-8d",  0, 0},     // audioDevice.out_block_count
    {1, 46, "%-8d",  0, 0},     // diff_count
    {1, 68, "%-8d",  0, 0},     // wrong_irq_count
    {2, 12, "%-8d",  0, 0},     // audioDevice.overflow_count
    {2, 28, "%-8d",  0, 0},     // audioDevice.underflow_count
};

#define NUM_STATIC_ENTRIES ((int) (sizeof(static_form) / sizeof(staticFormEntry)))
#define NUM_FORM_ENTRIES   ((int) (sizeof(form) / sizeof(formEntry)))


s32 block_diff = 0; 


statusScreen::statusScreen(CScreenDevice *pScreen)
{
    screen = pScreen;
    initialized = false;
    
    form[0].p_value = &bcm_pcm.in_block_count;
    form[1].p_value = &bcm_pcm.out_block_count;
    form[2].p_value = (u32 *) &block_diff;
    form[3].p_value = &bcm_pcm.wrong_irq_count;
    form[4].p_value = &bcm_pcm.overflow_count;
    form[5].p_value = &bcm_pcm.underflow_count;
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
    initialized = true;
}


void statusScreen::update()
{
    if (!initialized)
    {
        init();
        return;
    }
    
    block_diff = ((s32)bcm_pcm.out_block_count) - ((s32)bcm_pcm.in_block_count);
    
    for (int i=0; i<NUM_FORM_ENTRIES; i++)
    {
        formEntry *e = &form[i];
        if (e->last_value != *e->p_value)
        {
            e->last_value = *e->p_value;
            cursor(e->x,e->y);
            print_screen(e->format,e->last_value);
        }
    }

    #if 0
        int y = 5;
        for (AudioStream *p = AudioStream::first_update; p; p = p->next_update)
        {
            cursor(0,y++);    
            print_screen("%s%d\n",p->dbgName(),p->dbgInstance());
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
}
