/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include "battclock_intern.h"

static const char *Symbols[] = {
    "GetLocalTime",
    "SetLocalTime",
    NULL
};

/* auto init */
static int BattClock_Init(struct BattClockBase *BattClockBase)
{
    APTR HostLibBase;
    ULONG r;

    HostLibBase = OpenResource("hostlib.resource");
    D(bug("[battclock] HostLibBase = 0x%08lX\n", HostLibBase));
    if (HostLibBase) {
        BattClockBase->Lib = HostLib_Open("kernel32.dll", NULL);
        if (BattClockBase->Lib) {
    	    BattClockBase->KernelIFace = (struct KernelInterface *)HostLib_GetInterface(BattClockBase->Lib, Symbols, &r);
    	    D(bug("[battclock] KernelIFace = 0x%08lX\n", BattClockBase->KernelIFace));
    	    if (BattClockBase->KernelIFace) {
    	        if (!r)
    	            return 1;
    	        HostLib_DropInterface((APTR)BattClockBase->KernelIFace);
    	    }
    	    HostLib_Close(BattClockBase->Lib, NULL);
    	}
    }
    return 0;
}

ADD2INITLIB(BattClock_Init, 0)
