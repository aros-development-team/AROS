#ifndef DEFINES_TIMER_H
#define DEFINES_TIMER_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define AddTime(dest, src) \
    AROS_LC2(void, AddTime, \
    AROS_LCA(struct timeval *, dest, A0), \
    AROS_LCA(struct timeval *, src, A1), \
    struct Device *, TimerBase, 7, Timer)

#define CmpTime(dest, src) \
    AROS_LC2(LONG, CmpTime, \
    AROS_LCA(struct timeval *, dest, A0), \
    AROS_LCA(struct timeval *, src, A1), \
    struct Device *, TimerBase, 9, Timer)

#define GetSysTime(dest) \
    AROS_LC1(void, GetSysTime, \
    AROS_LCA(struct timeval *, dest, A0), \
    struct TimerBase *, TimerBase, 11, Timer)

#define ReadEClock(dest) \
    AROS_LC1(ULONG, ReadEClock, \
    AROS_LCA(struct EClockVal *, dest, A0), \
    struct TimerBase *, TimerBase, 10, Timer)

#define SubTime(dest, src) \
    AROS_LC2(void, SubTime, \
    AROS_LCA(struct timeval *, dest, A0), \
    AROS_LCA(struct timeval *, src, A1), \
    struct Device *, TimerBase, 8, Timer)


#endif /* DEFINES_TIMER_H */
