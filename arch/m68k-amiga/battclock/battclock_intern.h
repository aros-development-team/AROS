/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.

    Desc: Internal data structures for battclock.resource
    Lang: english
*/

#ifndef BATTCLOCK_INTERN_H
#define BATTCLOCK_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif

#define MSM6242B 1
#define RF5C01A 2

struct BattClockBase
{
    struct Library bb_LibNode;
    struct UtilityBase *UtilityBase;
    volatile UBYTE *clockptr;
    UBYTE clocktype;
};

void resetbattclock(struct BattClockBase *Battclock);
UBYTE getreg(volatile UBYTE *p, UBYTE regnum);
void putreg(volatile UBYTE *p, UBYTE regnum, UBYTE v);
UBYTE getbcd(volatile UBYTE *p, UBYTE regnum);

#endif //BATTCLOCK_INTERN_H
