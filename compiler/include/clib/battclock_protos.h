#ifndef CLIB_BATTCLOCK_PROTOS_H
#define CLIB_BATTCLOCK_PROTOS_H

/*
	Copyright 1995-1997 AROS - The Amiga Replacement OS
	$Id$

	Desc: Prototypes for battclock.resource
	Lang: english
*/

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

/*
    Prototypes
*/

AROS_LP0(ULONG, ReadBattClock,
    APTR, BattClockBase, 2, Battclock)

AROS_LP0(void, ResetBattClock,
    APTR, BattClockBase, 1, Battclock)

AROS_LP1(void, WriteBattClock,
    AROS_LPA(ULONG, time, D0),
    APTR *, BattClockBase, 3, Battclock)


#endif /*  */
