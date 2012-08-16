/*
    Copyright � 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Realtime.library initialization code.
    Lang: English.
*/

/* HISTORY:      25.06.99   SDuvan  Began implementing... */



#include <utility/utility.h>

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <aros/symbolsets.h>

#include <exec/libraries.h>

#include "realtime_intern.h"
#include LC_LIBDEFS_FILE

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

BOOL AllocTimer(struct internal_RealTimeBase *RealTimeBase);
void FreeTimer(struct internal_RealTimeBase *RealTimeBase);

extern void Pulse();

static int Init(LIBBASETYPEPTR LIBBASE)
{
    /* This function is single-threaded by exec by calling Forbid. */

    WORD   i;			/* Loop variable */

    for(i = 0; i < RT_MAXLOCK; i++)
    {
	InitSemaphore(&RealTimeBase->rtb_Locks[i]);
    }

    NEWLIST(&RealTimeBase->rtb_ConductorList);

    RealTimeBase->rtb_TickErr = 0;	/* How may such a thing be measured? */

    /* I use a process here just to be able to use CreateNewProc() so
       I don't have to fiddle with stack order and such... */
    {
	struct TagItem tags[] = { { NP_Entry   , (IPTR)Pulse            },
	                          { NP_Name    , (IPTR)"RealTime Pulse" },
				  { NP_Priority, (IPTR)127              },
			          { NP_UserData, (IPTR)RealTimeBase     },
			          { TAG_DONE   , (IPTR)NULL             } };
	
	GPB(RealTimeBase)->rtb_PulseTask = (struct Task *)CreateNewProc(tags);
    }
    
    if (GPB(RealTimeBase)->rtb_PulseTask == NULL)
    {
	return FALSE;
    }

    kprintf("Realtime pulse task created\n");

    if (!AllocTimer((struct internal_RealTimeBase *)RealTimeBase))
    {
	return FALSE;
    }
	
    kprintf("Realtime pulse timer created\n");

    return TRUE;
}


static int Expunge(LIBBASETYPEPTR LIBBASE)
{
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    FreeTimer(RealTimeBase);

    /* Shut down the pulse message task -- must be done AFTER freeing the
       timer! */
    Signal(RealTimeBase->rtb_PulseTask, SIGBREAKF_CTRL_C);
    
    return TRUE;
}


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
}


ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
