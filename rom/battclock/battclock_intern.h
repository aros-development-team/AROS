/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for battclock.resource and HIDD
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
#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif

struct BattClockBase
{
    struct Node		 bb_Node;
    struct ExecBase	*bb_SysBase;
    struct UtilityBase  *bb_UtilBase;
};

#define SysBase		(BattClockBase->bb_SysBase)
#ifdef UtilityBase
#   undef UtilityBase
#endif
#define UtilityBase     (BattClockBase->bb_UtilBase)

#endif //BATTCLOCK_INTERN_H
