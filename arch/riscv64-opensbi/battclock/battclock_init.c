/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: opensbi battclock.resource - stub implementation.

    SBI provides no real-time clock service; the wall clock comes from
    an RTC described in the device tree (goldfish-rtc on the qemu virt
    machine). Until that driver exists this resource just remembers
    whatever was written to it, so the rest of the system has a working
    (if not persistent) battery-backed clock.
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "battclock_intern.h"

static int BattClock_Init(struct BattClockBase *BattClockBase)
{
    BattClockBase->bb_Time = 0;

    return 1;
}

ADD2INITLIB(BattClock_Init, 0)
