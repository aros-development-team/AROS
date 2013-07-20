/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadBattClock() function.
    Lang: english
*/
#include "battclock_intern.h"

#include <proto/battclock.h>
#include <proto/utility.h>
#include <proto/rtas.h>
#include <proto/exec.h>
#include <utility/date.h>

/* See rom/battclock/readbattclock.c for documentation */

AROS_LH0(ULONG, ReadBattClock,
    struct BattClockBase *, BattClockBase, 2, Battclock)
{
    AROS_LIBFUNC_INIT

    struct ClockData date;
    void *RTASBase = OpenResource("rtas.resource");
    ULONG secs;
    ULONG out[8];

	RTASCall("get-time-of-day", 0, 8, out, NULL);

    date.year  = out[0];
    date.month = out[1];
    date.mday  = out[2];
    date.hour  = out[3];
    date.min   = out[4];
    date.sec   = out[5];

    secs=Date2Amiga(&date);

    return secs;

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
