// AudioMonitor.h
//
// A debugging class that uses the Circle HDMI Screen device
// and ansi escape codes to show statistics from the AudioSystem
// and bcm_pcm.  Client constructs in context of a CScreenDevice,
// which MUST be the HDMI device, and then periodically calls
// Update() ... at a UI frame rate 30/frames a sec or so.

#include "AudioMonitor.h"
#include "AudioSystem.h"
#include "bcm_pcm.h"
#include <utils/myUtils.h>


typedef struct
{
	const char *name;
	u32 last_val;
	u32 *ptr;
	u32 (*getter)();

} monitorVal_t;


bool inited;
u32 last_diff;


monitorVal_t mon_vals[] = {
	{ "cpuCycles",			0,	0,	AudioSystem::getCPUCycles,  			},
	{ "cpuCyclesMax",		0,	0,	AudioSystem::getCPUCyclesMax,			},
	{ "totalBlocks",		0,	0,	AudioSystem::getTotalMemoryBlocks,		},
	{ "blocksUsed",			0,	0,	AudioSystem::getMemoryBlocksUsed	    },
	{ "blocksUsedMax",		0,	0,	AudioSystem::getMemoryBlocksUsedMax,	},
	{ "in_irq_count",		0,	&bcm_pcm.in_irq_count,     },
	{ "out_irq_count",      0,	&bcm_pcm.out_irq_count,    },
	{ "in_block_count",     0,	&bcm_pcm.in_block_count,   },
	{ "in_other_count",     0,	&bcm_pcm.in_other_count,   },
	{ "in_wrong_count",     0,	&bcm_pcm.in_wrong_count,   },
	{ "out_block_count",    0,	&bcm_pcm.out_block_count,  },
	{ "out_other_count",    0,	&bcm_pcm.out_other_count,  },
	{ "out_wrong_count",    0,	&bcm_pcm.out_wrong_count,  },
	{ "underflow_count",    0,	&bcm_pcm.underflow_count,  },
	{ "overflow_count",     0,	&bcm_pcm.overflow_count,   },
	{ "diff_count",     	0,	&last_diff,   },
};





void AudioMonitor::Update()
{
	last_diff = bcm_pcm.out_block_count;
	last_diff -= bcm_pcm.in_block_count;

	int num = sizeof(mon_vals) / sizeof(monitorVal_t);
	printf("\x1b[J");
	printf("\x1b[1;1H");		// goto 0,0
	printf("AudioMonitor");

	#define VAL_Y		3		// 1 based
	#define VAL_X		20
	#define VAL_W		8

	for (int i=0; i<num; i++)
	{
		monitorVal_t *mon = &mon_vals[i];

		if (!inited)
		{
			printf("\x1b[%d;1H",i+VAL_Y); // cursor move y,x
			printf("%s",mon->name);
		}

		u32 val = mon->getter ? mon->getter() : *mon->ptr;
		if (val != mon->last_val)
		{
			mon->last_val = val;
			printf("\x1b[%d;%dH",i+VAL_Y,VAL_X);	// cursor move y,x
			printf("\x1b[%dX",VAL_W);				// erase N chars
			printf("%ld",val);
		}
	}


	// somebody is clearing the screen after I write the init values
	// I think it may be the blinking cursor from m_screen itself.
	
	printf("\x1b[%d;1H",num+3+VAL_Y);		// cursor move y,x

	inited = true;
}

//	\x1b[B		Cursor down one line
//	\x1b[H		Cursor home
//	\x1b[A		Cursor up one line
//	\x1b[%d;%dH	Cursor move to row %1 and column %2 (starting at 1)
//	^H		Cursor left one character
//	\x1b[D		Cursor left one character
//	\x1b[C		Cursor right one character
//	^M		Carriage return
//	\x1b[J		Clear to end of screen
//	\x1b[K		Clear to end of line
//	\x1b[%dX		Erase %1 characters starting at cursor
//	^J		Carriage return/linefeed
//	\x1b[0m		End of bold or half bright mode
//	\x1b[1m		Start bold mode
//	\x1b[2m		Start half bright mode
//	^I		Move to next hardware tab
//	\x1b[?25h		Normal cursor visible
//	\x1b[?25l		Cursor invisible
//	\x1b[%d;%dr	Set scroll region from row %1 to row %2 (starting at 1)

