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
    {  0,  12,  "IN          OUT" },
    {  2,  0,   "Blocks" },
    {  3,  0,   "Other" },
    {  4,  0,   "Wrong" },
    {  5,  0,   "Ov/Und" },
    {  6,  0,   "Diff" },
    
};


formEntry form[] =
{
    {0, 0,  "%-8d",  0, 0},     // main_loop_counter
    {2, 12, "%-8d",  0, 0},     // in_block_count
    {2, 24, "%-8d",  0, 0},     // out_block_count
    {3, 12, "%-8d",  0, 0},     // in_other_count
    {3, 24, "%-8d",  0, 0},     // out_other_count
    {4, 12, "%-8d",  0, 0},     // in_wrong_count
    {4, 24, "%-8d",  0, 0},     // out_wrong_count
    {5, 12, "%-8d",  0, 0},     // overflow_count
    {5, 24, "%-8d",  0, 0},     // underflow_count
    {6, 12, "%-8d",  0, 0},     // diff
};

#define NUM_STATIC_ENTRIES ((int) (sizeof(static_form) / sizeof(staticFormEntry)))
#define NUM_FORM_ENTRIES   ((int) (sizeof(form) / sizeof(formEntry)))


s32 block_diff = 0; 


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
