/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _INLINE_BATTCLOCK_H
#define _INLINE_BATTCLOCK_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef BATTCLOCK_BASE_NAME
#define BATTCLOCK_BASE_NAME BattClockBase
#endif

#define ReadBattClock() \
	LP0(0xc, ULONG, ReadBattClock, \
	, BATTCLOCK_BASE_NAME)

#define ResetBattClock() \
	LP0NR(0x6, ResetBattClock, \
	, BATTCLOCK_BASE_NAME)

#define WriteBattClock(time) \
	LP1NR(0x12, WriteBattClock, unsigned long, time, d0, \
	, BATTCLOCK_BASE_NAME)

#endif /* _INLINE_BATTCLOCK_H */
