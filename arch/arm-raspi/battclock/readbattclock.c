/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Desc: ReadBattClock() function, Raspberry Pi hardware RTC & file-backed fallback.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>
#include <proto/mbox.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include "battclock_intern.h"

AROS_LH0(ULONG, ReadBattClock,
         struct BattClockBase *, BattClockBase, 2, Battclock)
{
    AROS_LIBFUNC_INIT

    struct DosLibrary *DOSBase;
    APTR KernelBase;
    APTR MBoxBase;
    ULONG secs = 0;

    /*
     * 1. Query Hardware RTC via VideoCore Mailbox (e.g. Raspberry Pi 5 PMIC).
     */
    KernelBase = OpenResource("kernel.resource");
    MBoxBase = OpenResource("mbox.resource");

    if (KernelBase && MBoxBase)
    {
        IPTR peri_base = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);
        if (peri_base)
        {
            ULONG *msg = AllocMem(32, MEMF_PUBLIC | MEMF_CLEAR);
            if (msg)
            {
                msg[0] = 7 * sizeof(ULONG);  /* Buffer size */
                msg[1] = 0;                  /* Request */
                msg[2] = PROPTAG_GET_RTC;    /* Tag */
                msg[3] = 4;                  /* Value buffer size */
                msg[4] = 0;                  /* Request/response flag */
                msg[5] = 0;                  /* Clock ID (0) / output timestamp */
                msg[6] = 0;                  /* End tag */

                if (MBoxCall((void *)(peri_base + 0x00b880UL), VCMB_PROPCHAN, msg))
                {
                    ULONG posix_secs = msg[5];
                    if (posix_secs > AMIGA_POSIX_EPOCH_DIFF)
                    {
                        secs = posix_secs - AMIGA_POSIX_EPOCH_DIFF;
                        D(bug("[battclock] ReadBattClock: hardware RTC returned %lu (Amiga %lu)\n", posix_secs, secs));
                        FreeMem(msg, 32);
                        return secs;
                    }
                }
                FreeMem(msg, 32);
            }
        }
    }

    /*
     * 2. Fallback: For older boards without hardware RTC, read from DEVS:battclock.
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
