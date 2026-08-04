/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: WriteBattClock() function.
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

/* See rom/battclock/writebattclock.c for documentation */

static void WriteClockData(struct BattClockBase *BattClockBase,
                           struct ClockData *date)
{
    UBYTE buf[2 + M41T11_CLOCK_REGS];

    if (!BattClockBase->bb_Bus)
        return;

    /*
     * The part has two digits of year and no century, so a date outside
     * the hundred years m41t11.h picks cannot be stored. ResetBattClock()
     * asks for the epoch itself, 1978, and lands here.
     */
    if (date->year < M41T11_CENTURY || date->year >= M41T11_CENTURY + 100)
    {
        D(bug("[BattClock] %u is outside what the RTC can hold\n",
              date->year));
        return;
    }

    /*
     * Written as one transfer starting at register 0, so the part takes
     * the whole time at once instead of being caught part way through
     * being changed. The fraction of a second goes back to zero, which
     * is as close to "now" as the caller's whole seconds can get.
     */
    buf[0] = M41T11_REG_HSECS;
    buf[1] = 0;

    /* ST left clear, so the oscillator runs on from here */
    buf[2] = m41t11_ToBCD(date->sec);
    buf[3] = m41t11_ToBCD(date->min);
    buf[4] = m41t11_ToBCD(date->hour);

    /*
     * The part never interprets the weekday register, it only counts it
     * 1 to 7, so all that matters is that it agrees with the date.
     */
    buf[5] = (date->wday + 1) & M41T11_WDAY_MASK;

    buf[6] = m41t11_ToBCD(date->mday);
    buf[7] = m41t11_ToBCD(date->month);
    buf[8] = m41t11_ToBCD(date->year - M41T11_CENTURY);

    if (!BattClock_I2CWrite(BattClockBase, buf, sizeof(buf)))
    {
        D(bug("[BattClock] the RTC would not take the time\n"));
    }
}

AROS_LH1(void, WriteBattClock,
         AROS_LHA(ULONG, time, D0),
         struct BattClockBase *, BattClockBase, 3, Battclock)
{
    AROS_LIBFUNC_INIT

    struct ClockData date;

    Amiga2Date(time, &date);

    ObtainSemaphore(&BattClockBase->bb_Lock);

    /*
     * Kept whether or not there is an RTC to take it: a machine without
     * one still has to read back what it was told.
     */
    BattClockBase->bb_Time = time;

    WriteClockData(BattClockBase, &date);

    ReleaseSemaphore(&BattClockBase->bb_Lock);

    AROS_LIBFUNC_EXIT
} /* WriteBattClock */
