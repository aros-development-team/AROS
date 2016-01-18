/*
    Copyright � 1995-2016, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Realtime.library initialization code.
    Lang: English.
*/

/* HISTORY:      25.06.99   SDuvan  Began implementing... */

#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include <exec/libraries.h>

#include "realtime_intern.h"
#include LC_LIBDEFS_FILE

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

BOOL AllocTimer(struct internal_RealTimeBase *RealTimeBase);
void FreeTimer(struct internal_RealTimeBase *RealTimeBase);

static int Init(LIBBASETYPEPTR LIBBASE)
{
    /* This function is single-threaded by exec by calling Forbid. */

    WORD   i;			/* Loop variable */

    for(i = 0; i < RT_MAXLOCK; i++)
    {
	InitSemaphore(&RealTimeBase->rtb_Locks[i]);
    }

    NEWLIST(&RealTimeBase->rtb_ConductorList);

    if (!AllocTimer((struct internal_RealTimeBase *)RealTimeBase))
    {
	return FALSE;
    }

    D(bug("[realtime.library] initialized\n"));

    return TRUE;
}


static int Expunge(LIBBASETYPEPTR LIBBASE)
{
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    FreeTimer(RealTimeBase);

    return TRUE;
}


ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
