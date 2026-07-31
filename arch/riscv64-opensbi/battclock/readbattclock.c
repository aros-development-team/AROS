/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: ReadBattClock() function.
*/

#include <proto/battclock.h>

#include "battclock_intern.h"

/* See rom/battclock/readbattclock.c for documentation */

AROS_LH0(ULONG, ReadBattClock,
         struct BattClockBase *, BattClockBase, 2, Battclock)
{
    AROS_LIBFUNC_INIT

    /* No RTC driver yet - report whatever was last written */
    return BattClockBase->bb_Time;

    AROS_LIBFUNC_EXIT
} /* ReadBattClock */
