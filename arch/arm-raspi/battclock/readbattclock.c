/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ReadBattClock() function, Raspberry Pi file-backed version.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include "battclock_intern.h"

AROS_LH0(ULONG, ReadBattClock,
         struct BattClockBase *, BattClockBase, 2, Battclock)
{
    AROS_LIBFUNC_INIT

    struct DosLibrary *DOSBase;
    ULONG secs = 0;

    /*
     * The Raspberry Pi has no battery-backed RTC, so the clock is kept in a
     * plain file on the boot volume (written by WriteBattClock()). The clock
     * does not tick while the file is at rest; it simply preserves the value
     * SetClock SAVE last stored.
     */
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    if (DOSBase)
    {
        BPTR fh;

        fh = Open(BATTCLOCK_FILE, MODE_OLDFILE);
        if (fh)
        {
            ULONG val;
            LONG n;

            n = Read(fh, &val, sizeof(val));
            if (n == sizeof(val))
                secs = val;

            Close(fh);
        }

        CloseLibrary((struct Library *)DOSBase);
    }

    D(bug("[battclock] ReadBattClock: returning %lu\n", secs));
    return secs;

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
