/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    ChildWait() - Wait for a task to finish it processing.
*/
#include "exec_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

        AROS_LH1(IPTR, ChildWait,

/*  SYNOPSIS */
        AROS_LHA(ULONG, tid, D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 126, Exec)

/*  FUNCTION
        Wait for either a specific child task, or any child task to finish.
        If you specify tid = 0, then the call will return when any child
        task exits, otherwise it will not return until the requested task
        finishes.

        Note that the tid is NOT the task control block (ie struct Task *),
        rather it is the value of the ETask et_UniqueID field. Passing in a
        Task pointer will cause your Task to deadlock.

        You must call ChildFree() on the returned task before calling
        ChildWait() again. Ie.

            struct ETask *et;

            et = ChildWait(0);
            ChildFree(et->et_UniqueID);

    INPUTS
        tid -   The UniqueID of a task.

    RESULT
        Returns either the ETask structure of the child, or one of the
        CHILD_* values on error.

        This allows you to work out which of the children has exited.

    NOTES
        This function will work correctly only for child tasks that are
        processes created with NP_NotifyOnDeath set to TRUE.

        Calling ChildWait() on a task that isn't your child will result in
        a deadlock.

    EXAMPLE

    BUGS
        Be careful with the return result of this function.

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *ThisTask = GET_THIS_TASK;
    struct ETask *et;
    struct ETask *child;

    /*
        ChildWait() will wait for either a specific or any child task to
        finish. We we get it we return the task unique id to the caller.

        Firstly, are we a new-style Task?
    */
    if (!ThisTask || (ThisTask->tc_Flags & TF_ETASK) == 0)
        return CHILD_NOTNEW;

    et = ThisTask->tc_UnionETask.tc_ETask;

    /*
        Scanning the msgport list is unsafe, we need to Forbid().
           Note that the Wait() below will break the Forbid() condition.
           This is how we need it to be.
    */
    Forbid();
    /* We do not return until the condition is met */
    for (;;)
    {
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_LOCK(&et->et_TaskMsgPort.mp_SpinLock, NULL, SPINLOCK_MODE_READ);
#endif
        /* Check if it has returned already. This will also take the first. */
        ForeachNode(&et->et_TaskMsgPort.mp_MsgList, child)
        {
            if (tid == 0 || child->et_UniqueID == tid)
            {
#if defined(__AROSEXEC_SMP__)
                EXEC_SPINLOCK_UNLOCK(&et->et_TaskMsgPort.mp_SpinLock);
#endif
                goto child_exited;
            }
        }

#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_UNLOCK(&et->et_TaskMsgPort.mp_SpinLock);
#endif
        /* No matching children, we have to wait */
        SetSignal(0, SIGF_CHILD);
        Wait(SIGF_CHILD);
    }

child_exited:
    Permit();
    return (IPTR)child;

    AROS_LIBFUNC_EXIT
} /* ChildWait */
