/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef TASKRES_INTERN_H
#define TASKRES_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif

struct TaskResBase
{
    struct Library              trb_LibNode;
    APTR                        trb_KernelBase;
    struct List                 trb_TaskLists;
};

struct TaskListEntry
{
    struct Node                 tle_Node;
    struct Task                 *tle_Task;
};

// The "Real" implementation of struct TaskList
struct TaskListPrivate
{
    struct Node                 tlp_Node;
    struct List                 tlp_Tasks;
    struct TaskListEntry        *tlp_Next;
};

#ifdef KernelBase
#undef KernelBase
#endif

#define KernelBase TaskResBase->trb_KernelBase

#endif /* TASKRES_INTERN_H */
