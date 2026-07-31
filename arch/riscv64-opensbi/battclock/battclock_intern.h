/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Internal data structures for the opensbi battclock.resource
*/

#ifndef BATTCLOCK_INTERN_H
#define BATTCLOCK_INTERN_H

#include <exec/libraries.h>

struct BattClockBase
{
    struct Library  bb_LibNode;
    ULONG           bb_Time;    /* the value last written, see below */
};

#endif /* BATTCLOCK_INTERN_H */
