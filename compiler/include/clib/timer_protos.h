#ifndef CLIB_TIMER_PROTOS_H
#define CLIB_TIMER_PROTOS_H

/*
	Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
	$Id$

	Desc: Prototypes for timer.device
	Lang:
*/

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP2(void, AddTime,
    AROS_LPA(struct timeval *, dest, A0),
    AROS_LPA(struct timeval *, src, A1),
    struct Device *, TimerBase, 7, Timer)

AROS_LP2(LONG, CmpTime,
    AROS_LPA(struct timeval *, dest, A0),
    AROS_LPA(struct timeval *, src, A1),
    struct Device *, TimerBase, 9, Timer)

AROS_LP1(void, GetSysTime,
    AROS_LPA(struct timeval *, dest, A0),
    struct TimerBase *, TimerBase, 11, Timer)

AROS_LP1(ULONG, ReadEClock,
    AROS_LPA(struct EClockVal *, dest, A0),
    struct TimerBase *, TimerBase, 10, Timer)

AROS_LP2(void, SubTime,
    AROS_LPA(struct timeval *, dest, A0),
    AROS_LPA(struct timeval *, src, A1),
    struct Device *, TimerBase, 8, Timer)


#endif /* CLIB_TIMER_PROTOS_H */
