#ifndef DEFINES_BATTCLOCK_H
#define DEFINES_BATTCLOCK_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define ReadBattClock() \
    AROS_LC0(ULONG, ReadBattClock, \
    APTR, BattClockBase, 2, Battclock)

#define ResetBattClock() \
    AROS_LC0(void, ResetBattClock, \
    APTR, BattClockBase, 1, Battclock)

#define WriteBattClock(time) \
    AROS_LC1(void, WriteBattClock, \
    AROS_LCA(ULONG, time, D0), \
    APTR *, BattClockBase, 3, Battclock)


#endif /* DEFINES_BATTCLOCK_H */
