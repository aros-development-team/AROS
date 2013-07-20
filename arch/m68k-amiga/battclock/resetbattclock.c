/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.

    Desc: ResetBattClock()
    Lang: english
*/
#include "battclock_intern.h"

#include <proto/battclock.h>

/* See rom/battclock/resetbattclock.c for documentation */

AROS_LH0(void, ResetBattClock,
    struct BattClockBase*, BattClockBase, 1, Battclock)
{
    AROS_LIBFUNC_INIT

    resetbattclock(BattClockBase);
    WriteBattClock(0);

    AROS_LIBFUNC_EXIT
} /* ResetBattClock */
