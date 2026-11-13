/*
    Copyright (C) 2015-2016, The AROS Development Team. All rights reserved.

    Desc: timer implementation.
*/

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <exec/interrupts.h>
#include <hardware/intbits.h>

#include "realtime_intern.h"

extern void Pulse();

/* RealTime timer interrupt -- currently only a VBlank interrupt */
static AROS_INTH1(rtVBlank, struct internal_RealTimeBase *, RealTimeBase)
{
    AROS_INTFUNC_INIT

    // kprintf("Signalling task %p\n", RealTimeBase->rtb_PulseTask);
    Signal(RealTimeBase->rtb_PulseTask, SIGF_SINGLE);

    return 0;

    AROS_INTFUNC_EXIT
}


BOOL AllocTimer(struct internal_RealTimeBase *RealTimeBase)
{
    RealTimeBase->rtb_TickErr = 0;      /* How may such a thing be measured?            */
    RealTimeBase->rtb_Reserved1 = 60;   /* NB: rtb_Reserved1 contains the TICK_FREQ,
                                         * which in our case is vblank (60)             */

    /* I use a process here just to be able to use CreateNewProc() so
       I don't have to fiddle with stack order and such... */
    {
        struct TagItem tags[] = { { NP_Entry   , (IPTR)Pulse            },
                                  { NP_Name    , (IPTR)"RealTime Pulse" },
                                  { NP_Priority, (IPTR)127              },
                                  { NP_UserData, (IPTR)RealTimeBase     },
                                  { TAG_DONE   , (IPTR)NULL             } };
        
        RealTimeBase->rtb_PulseTask = (struct Task *)CreateNewProc(tags);
    }
    
    if (RealTimeBase->rtb_PulseTask == NULL)
    {
        return FALSE;
    }

    D(bug("[realtime.library] pulse task created @ 0x%p\n", GPB(RealTimeBase)->rtb_PulseTask));

    /* TODO */
    /* This should be replaced by some timer.device thing when an accurate
       timer is available -- UNIT_MICROHZ? */

    RealTimeBase->rtb_VBlank.is_Code         = (APTR)&rtVBlank;
    RealTimeBase->rtb_VBlank.is_Data         = (APTR)RealTimeBase;
    RealTimeBase->rtb_VBlank.is_Node.ln_Name = "RealTime VBlank server";
    RealTimeBase->rtb_VBlank.is_Node.ln_Pri  = 127;
    RealTimeBase->rtb_VBlank.is_Node.ln_Type = NT_INTERRUPT;
    
    /* Add a VBLANK server to take care of the heartbeats. */
    AddIntServer(INTB_VERTB, &RealTimeBase->rtb_VBlank);

    return TRUE;
}


void FreeTimer(struct internal_RealTimeBase *RealTimeBase)
{
    RemIntServer(INTB_VERTB, &RealTimeBase->rtb_VBlank);

    if (RealTimeBase->rtb_PulseTask)
        Signal(RealTimeBase->rtb_PulseTask, SIGBREAKF_CTRL_C);
}
