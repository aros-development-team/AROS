#define DEBUG 1

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/hostlib.h>

#include <time.h>

#include "battclock_intern.h"

static const char *Symbols[] = {
    "time",
    "localtime",
    NULL
};

/* auto init */
static int BattClock_Init(struct BattClockBase *BattClockBase)
{
    APTR HostLibBase;
    ULONG r;

    HostLibBase = OpenResource("hostlib.resource");
    D(bug("[battclock] HostLibBase = 0x%08lX\n", HostLibBase));

    if (HostLibBase)
    {
        BattClockBase->Lib = HostLib_Open("libc.so.6", NULL);
        if (BattClockBase->Lib)
	{
    	    BattClockBase->SysIFace = (struct BattclockInterface *)HostLib_GetInterface(BattClockBase->Lib, Symbols, &r);
    	    D(bug("[battclock] SysIFace = 0x%08lX, unresolved: %u\n", BattClockBase->SysIFace, r));

    	    if (BattClockBase->SysIFace)
	    {
    	        if (!r)
    	            return 1;
    	        HostLib_DropInterface((APTR)BattClockBase->SysIFace);
    	    }
    	    HostLib_Close(BattClockBase->Lib, NULL);
    	}
    }
    return 0;
}

ADD2INITLIB(BattClock_Init, 0)
