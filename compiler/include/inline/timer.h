#ifndef _INLINE_TIMER_H
#define _INLINE_TIMER_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef TIMER_BASE_NAME
#define TIMER_BASE_NAME TimerBase
#endif

#define AddTime(dest, src) \
	LP2NR(0x2a, AddTime, struct timeval *, dest, a0, struct timeval *, src, a1, \
	, TIMER_BASE_NAME)

#define CmpTime(dest, src) \
	LP2(0x36, LONG, CmpTime, struct timeval *, dest, a0, struct timeval *, src, a1, \
	, TIMER_BASE_NAME)

#define GetSysTime(dest) \
	LP1NR(0x42, GetSysTime, struct timeval *, dest, a0, \
	, TIMER_BASE_NAME)

#define ReadEClock(dest) \
	LP1(0x3c, ULONG, ReadEClock, struct EClockVal *, dest, a0, \
	, TIMER_BASE_NAME)

#define SubTime(dest, src) \
	LP2NR(0x30, SubTime, struct timeval *, dest, a0, struct timeval *, src, a1, \
	, TIMER_BASE_NAME)

#endif /* _INLINE_TIMER_H */
