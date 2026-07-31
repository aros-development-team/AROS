/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: WriteBattClock() function.
*/

#include <proto/battclock.h>

#include "battclock_intern.h"

/* See rom/battclock/writebattclock.c for documentation */

AROS_LH1(void, WriteBattClock,
         AROS_LHA(ULONG, time, D0),
         struct BattClockBase *, BattClockBase, 3, Battclock)
{
    AROS_LIBFUNC_INIT

    /* No RTC driver yet - the value is not persistent */
    BattClockBase->bb_Time = time;

    AROS_LIBFUNC_EXIT
} /* WriteBattClock */
