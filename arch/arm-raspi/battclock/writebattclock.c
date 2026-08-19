/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Desc: WriteBattClock() function, Raspberry Pi hardware RTC & file-backed fallback.
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

AROS_LH1(void, WriteBattClock,
         AROS_LHA(ULONG, time, D0),
         struct BattClockBase *, BattClockBase, 3, Battclock)
{
    AROS_LIBFUNC_INIT

    struct DosLibrary *DOSBase;
    APTR KernelBase;
    APTR MBoxBase;

    /*
     * 1. Write to Hardware RTC via VideoCore Mailbox (Raspberry Pi 5 PMIC).
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
                msg[0] = 7 * sizeof(ULONG);                       /* Buffer size */
                msg[1] = 0;                                       /* Request */
                msg[2] = PROPTAG_SET_RTC;                         /* Tag */
                msg[3] = 4;                                       /* Value buffer size */
                msg[4] = 0;                                       /* Request flag */
                msg[5] = time + AMIGA_POSIX_EPOCH_DIFF;          /* POSIX timestamp */
                msg[6] = 0;                                       /* End tag */

                MBoxCall((void *)(peri_base + 0x00b880UL), VCMB_PROPCHAN, msg);
                FreeMem(msg, 32);
            }
        }
    }

    /*
     * 2. Persist the time to the boot volume as fallback; see ReadBattClock().
     */
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
