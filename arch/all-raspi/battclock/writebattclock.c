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
#include "battclock_rtc.h"
#include "battclock_ds3231.h"

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
        ULONG posix_secs = time + AMIGA_POSIX_EPOCH_DIFF;
        int ok = rpi_rtc_reg(KernelBase, MBoxBase, PROPTAG_SET_RTC,
                             RPI_RTC_REG_TIME, &posix_secs);

        D(bug("[battclock] WriteBattClock: hardware RTC set %u -> %d\n",
              posix_secs, ok));
        (void)ok;
    }

    /*
     * 2. DS3231 on the i2c header, if the device tree has one.
     */
    if (DS3231_Write(time))
        D(bug("[battclock] WriteBattClock: DS3231 set\n"));

    /*
     * 3. Persist the time to the boot volume as fallback; see ReadBattClock().
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
