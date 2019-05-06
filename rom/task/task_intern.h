/*
    Copyright © 2015-2019, The AROS Development Team. All rights reserved.
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
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif

#include <exec_intern.h>

#define TASKRES_ENABLE

#if defined(__AROSEXEC_SMP__)
#include <aros/types/spinlock_s.h>
#include <resources/execlock.h>
#ifndef TASKRES_ENABLE
#define TASKRES_ENABLE
#endif
#endif

struct TaskResBase
{
    struct Library              trb_LibNode;
    APTR                        trb_KernelBase;
    APTR                        trb_NewAddTask;
    APTR                        trb_RemTask;
    struct SignalSemaphore      trb_Sem;
#if defined(__AROSEXEC_SMP__)
    spinlock_t                  TaskListSpinLock;
    void *                      trb_ExecLock;
#endif
    struct List                 trb_TaskList;
    struct List                 trb_NewTasks;
    struct List                 trb_LockedLists;
    struct Library *            trb_UtilityBase;
};

struct TaskListEntry
{
    struct Node                 tle_Node;
    struct Task                 *tle_Task;
    struct List                 tle_HookTypes;
};

struct TaskListHookEntry
{
    struct Node                 tlhe_Node;
    struct List                 tlhe_Hooks;
};

struct TaskListHookNode
{
    struct Hook                 *tln_Hook;
};

#ifdef TASKRES_ENABLE
struct TaskListPrivate
{
    struct Node                 tlp_Node;
    ULONG                       tlp_Flags;
    struct List                 *tlp_Tasks;
    struct TaskListEntry        *tlp_Next;
};
#else
struct TaskListPrivate
{
    struct List                 *tlp_TaskList;
    struct Task                *tlp_Current;
};
#endif /* TASKRES_ENABLE */

#ifdef KernelBase
#undef KernelBase
#endif

#define KernelBase TaskResBase->trb_KernelBase

void task_CleanList(struct Task * task, struct TaskResBase *TaskResBase);
struct TaskListHookEntry *GetHookTypeEntry(struct List *htList, ULONG thType, BOOL create);
struct TaskListEntry *GetTaskEntry(struct Task *thisTask, struct TaskResBase *TaskResBase);

#endif /* TASKRES_INTERN_H */
