/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for battclock.resource and HIDD
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif

struct BattClockBase
{
    struct Node		 bb_Node;
    struct ExecBase	*bb_SysBase;
};

#define SysBase		(BattClockBase->bb_SysBase)
