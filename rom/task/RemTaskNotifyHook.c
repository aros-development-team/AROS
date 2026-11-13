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

        AROS_LH1(VOID, RemTaskNotifyHook,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, hook, A0),

/*  LOCATION */
        struct TaskResBase *, TaskResBase, 20, Task)

/*  FUNCTION
        Remove a hook previously registered with AddTaskNotifyHook().

    INPUTS
        hook - the hook to remove.

    RESULT
        None. Once this function returns the hook is no longer called.

    SEE ALSO
        AddTaskNotifyHook()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!hook)
        return;

    ObtainSemaphore(&TaskResBase->trb_NotifySem);
    Remove((struct Node *)&hook->h_MinNode);
    ReleaseSemaphore(&TaskResBase->trb_NotifySem);

    D(bug("[TaskRes] %s: hook @ 0x%p removed\n", __func__, hook));

    AROS_LIBFUNC_EXIT
} /* RemTaskNotifyHook() */
