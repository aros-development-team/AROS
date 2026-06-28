/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Internal definitions for battclock.resource, Raspberry Pi version.
*/

#ifndef BATTCLOCK_INTERN_H
#define BATTCLOCK_INTERN_H

#include <exec/libraries.h>

/* File on the boot volume that persists the clock across reboots. */
#define BATTCLOCK_FILE "DEVS:battclock"

struct BattClockBase
{
    struct Library bb_LibNode;
};

#endif /* BATTCLOCK_INTERN_H */
