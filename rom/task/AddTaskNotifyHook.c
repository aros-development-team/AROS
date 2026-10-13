/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <aros/libcall.h>
#include <resources/task.h>

#include "task_intern.h"

/*****************************************************************************

    NAME */
#include <proto/task.h>

        AROS_LH1(BOOL, AddTaskNotifyHook,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, hook, A0),

/*  LOCATION */
        struct TaskResBase *, TaskResBase, 19, Task)

/*  FUNCTION
        Register a hook that is called whenever a task is added to, or
        removed from, the system. See struct TaskNotifyMsg in
        <resources/task.h> for the calling convention and the context the
        hook runs in.

    INPUTS
        hook - the hook to add. Its h_MinNode is used to link it.

    RESULT
        TRUE on success.

    NOTES
        The hook is called for every task in the system, keep it short.

    SEE ALSO
        RemTaskNotifyHook()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!hook)
        return FALSE;

    ObtainSemaphore(&TaskResBase->trb_NotifySem);
    AddTail((struct List *)&TaskResBase->trb_NotifyHooks, (struct Node *)&hook->h_MinNode);
    ReleaseSemaphore(&TaskResBase->trb_NotifySem);

    D(bug("[TaskRes] %s: hook @ 0x%p registered\n", __func__, hook));

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* AddTaskNotifyHook() */
