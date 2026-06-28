/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: WriteBattClock() function, Raspberry Pi file-backed version.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include "battclock_intern.h"

AROS_LH1(void, WriteBattClock,
         AROS_LHA(ULONG, time, D0),
         struct BattClockBase *, BattClockBase, 3, Battclock)
{
    AROS_LIBFUNC_INIT

    struct DosLibrary *DOSBase;

    /* Persist the time to the boot volume; see ReadBattClock() for rationale. */
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    if (DOSBase)
    {
        BPTR fh;

        fh = Open(BATTCLOCK_FILE, MODE_NEWFILE);
        if (fh)
        {
            LONG n;

            n = Write(fh, &time, sizeof(time));
            D(bug("[battclock] WriteBattClock: Write returned %ld\n", (long)n));
            Close(fh);
        }

        CloseLibrary((struct Library *)DOSBase);
    }

    AROS_LIBFUNC_EXIT
} /* WriteBattClock */
