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
#include "battclock_rtc.h"
#include "battclock_ds3231.h"

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
        ULONG posix_secs = 0;
        int ok = rpi_rtc_reg(KernelBase, MBoxBase, PROPTAG_GET_RTC,
                             RPI_RTC_REG_TIME, &posix_secs);

        if (ok && posix_secs > AMIGA_POSIX_EPOCH_DIFF)
        {
            secs = posix_secs - AMIGA_POSIX_EPOCH_DIFF;
            D(bug("[battclock] ReadBattClock: hardware RTC returned %u (Amiga %u)\n",
                  posix_secs, secs));
            return secs;
        }
        D(bug("[battclock] ReadBattClock: hardware RTC unusable (ok %d, value %u)\n",
              ok, posix_secs));
    }

    /*
     * 2. DS3231 on the i2c header (Pi 2/3/4), if the device tree has one.
     */
    if (DS3231_Read(&secs))
    {
        D(bug("[battclock] ReadBattClock: DS3231 returned %u\n", secs));
        return secs;
    }

    /*
     * 3. Fallback: For older boards without hardware RTC, read from DEVS:battclock.
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

    D(bug("[battclock] ReadBattClock: returning %u\n", secs));
    return secs;

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
