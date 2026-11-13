/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: ReadBattClock() function.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/battclock.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <utility/date.h>

#include "battclock_intern.h"
#include "battclock_i2c.h"
#include "m41t11.h"

/* See rom/battclock/readbattclock.c for documentation */

static BOOL ReadClockData(struct BattClockBase *BattClockBase,
                          struct ClockData *date)
{
    UBYTE regs[M41T11_CLOCK_REGS];
    UBYTE reg = M41T11_REG_SECS;

    if (!BattClockBase->bb_Bus)
        return FALSE;

    /*
     * One transfer for all seven registers: reading them one at a time
     * could catch the counters mid-tick and produce a time that never
     * existed, for instance 12:59:00 read as 12:00:00.
     */
    if (!BattClock_I2CWriteRead(BattClockBase, &reg, 1, regs, sizeof(regs)))
    {
        D(bug("[BattClock] the RTC did not answer\n"));
        return FALSE;
    }

    /* While ST is set the oscillator is held stopped, so this is stale */
    if (regs[0] & M41T11_SECS_ST)
    {
        D(bug("[BattClock] the RTC oscillator is stopped\n"));
        return FALSE;
    }

    date->sec   = m41t11_FromBCD(regs[0] & M41T11_SECS_MASK);
    date->min   = m41t11_FromBCD(regs[1] & M41T11_MINS_MASK);
    date->hour  = m41t11_FromBCD(regs[2] & M41T11_HOURS_MASK);
    date->mday  = m41t11_FromBCD(regs[4] & M41T11_MDAY_MASK);
    date->month = m41t11_FromBCD(regs[5] & M41T11_MONTH_MASK);
    date->year  = M41T11_CENTURY + m41t11_FromBCD(regs[6]);

    /* Date2Amiga() works the weekday out from the date itself */
    date->wday  = 0;

    /*
     * A flat backup battery, or a bus that dropped a bit, leaves digits
     * that are not a date at all - and Date2Amiga() would turn those
     * into a plausible looking time centuries out rather than complain.
     */
    if (date->sec > 59 || date->min > 59 || date->hour > 23 ||
        date->mday < 1 || date->mday > 31 ||
        date->month < 1 || date->month > 12)
    {
        D(bug("[BattClock] the RTC holds no sensible date\n"));
        return FALSE;
    }

    return TRUE;
}

AROS_LH0(ULONG, ReadBattClock,
         struct BattClockBase *, BattClockBase, 2, Battclock)
{
    AROS_LIBFUNC_INIT

    struct ClockData date;
    ULONG secs;

    ObtainSemaphore(&BattClockBase->bb_Lock);

    if (ReadClockData(BattClockBase, &date))
        secs = Date2Amiga(&date);
    else
    {
        /* No RTC, or nothing usable in it - the last write is all there is */
        secs = BattClockBase->bb_Time;
    }

    ReleaseSemaphore(&BattClockBase->bb_Lock);

    return secs;

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
