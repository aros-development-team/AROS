/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

    Free child task information on a dead child.
*/
#include "exec_intern.h"
#include "exec_util.h"
#include "etask.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

        AROS_LH1(void, ChildFree,

/*  SYNOPSIS */
        AROS_LHA(ULONG, tid, D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 123, Exec)

/*  FUNCTION
        Clean up after a child process.

    INPUTS
        tid     --  Id of the child to clean up. This is not the same as
                    the Task pointer.

    RESULT
        The child will be freed.

    NOTES
        This function will work correctly only for child tasks that are
        processes created with NP_NotifyOnDeath set to TRUE.

        Calling ChildFree() on a running child is likely to crash your
        system badly.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *ThisTask = GET_THIS_TASK;
    struct ETask *parent, *et = NULL, *child;

    Forbid();
    parent = ThisTask ? GetETask(ThisTask) : NULL;
    if (parent)
    {
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_LOCK(&IntETask(parent)->iet_TaskLock, NULL, SPINLOCK_MODE_WRITE);
#endif
        ForeachNode(&parent->et_Children, child)
        {
            if (child->et_UniqueID == tid)
            {
                et = child;
                Remove((struct Node *)et);
                break;
            }
        }
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_UNLOCK(&IntETask(parent)->iet_TaskLock);
#endif

        if (et == NULL)
        {
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_LOCK(&parent->et_TaskMsgPort.mp_SpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
            ForeachNode(&parent->et_TaskMsgPort.mp_MsgList, child)
            {
                if (child->et_UniqueID == tid)
                {
                    et = child;
                    Remove((struct Node *)et);
                    break;
                }
            }
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_UNLOCK(&parent->et_TaskMsgPort.mp_SpinLock);
#endif
        }
    }
    if(et != NULL)
        ExpungeETask(et);
    Permit();
    
    AROS_LIBFUNC_EXIT
} /* ChildFree */
