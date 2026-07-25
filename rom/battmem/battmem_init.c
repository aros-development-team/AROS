/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <resources/battclock.h>

#include "battmem_intern.h"

static int BattMem_Init(struct BattMemBase *BattMemBase)
{
    /*
       All the storage lives in battclock.resource, which owns the real
       time clock chip. Without it there is nowhere to put anything, so
       do not install the resource at all.
    */
    BattMemBase->bm_BattClockBase = OpenResource(BATTCLOCKNAME);
    if (!BattMemBase->bm_BattClockBase)
    {
        D(bug("BattMem: no %s\n", BATTCLOCKNAME));
        return FALSE;
    }

    InitSemaphore(&BattMemBase->bm_Semaphore);

    return TRUE;
}

ADD2INITLIB(BattMem_Init, 0)
