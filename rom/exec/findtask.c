/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Search a task by name.
*/

#define DEBUG 0

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_locks.h"

/*****************************************************************************

    NAME */

        AROS_LH1(struct Task *, FindTask,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 49, Exec)

/*  FUNCTION
        Find a task with a given name or get the address of the current task.
        Finding the address of the current task is a very quick function
        call, but finding a special task is a very CPU intensive instruction.
        Note that generally a task may already be gone when this function
        returns.

    INPUTS
        name - Pointer to name or NULL for current task.

    RESULT
        Address of task structure found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    spinlock_t *listlock;
#endif
    struct Task *ret, *thisTask = GET_THIS_TASK;

    /* Quick return for a quick argument */
    if (name == NULL)
        return thisTask;

    /* Always protect task lists */
#if defined(__AROSEXEC_SMP__)
    EXEC_LOCK_READ_AND_DISABLE(&PrivExecBase(SysBase)->TaskReadySpinLock);
    listlock = &PrivExecBase(SysBase)->TaskReadySpinLock;
#else
    Disable();
#endif

    /* First look into the ready list. */
    ret = (struct Task *)FindName(&SysBase->TaskReady, name);
    if (ret == NULL)
    {
#if defined(__AROSEXEC_SMP__)
        EXEC_UNLOCK_AND_ENABLE(listlock);
        EXEC_LOCK_READ_AND_DISABLE(&PrivExecBase(SysBase)->TaskWaitSpinLock);
        listlock = &PrivExecBase(SysBase)->TaskWaitSpinLock;
#endif
        /* Then into the waiting list. */
        ret = (struct Task *)FindName(&SysBase->TaskWait, name);
        if (ret == NULL)
        {
            /*
                Finally test the running task(s). This is mainly of importance on smp systems.
            */
#if defined(__AROSEXEC_SMP__)
            EXEC_UNLOCK_AND_ENABLE(listlock);
            EXEC_LOCK_READ_AND_DISABLE(&PrivExecBase(SysBase)->TaskRunningSpinLock);
            listlock = &PrivExecBase(SysBase)->TaskRunningSpinLock;
            ret = (struct Task *)FindName(&PrivExecBase(SysBase)->TaskRunning, name);
            if (ret == NULL)
            {
                EXEC_UNLOCK_AND_ENABLE(listlock);
                EXEC_LOCK_READ_AND_DISABLE(&PrivExecBase(SysBase)->TaskSpinningLock);
                listlock = &PrivExecBase(SysBase)->TaskSpinningLock;
                ret = (struct Task *)FindName(&PrivExecBase(SysBase)->TaskSpinning, name);
            }
#else

            char *s1;
            const char *s2 = name;
            s1 = thisTask->tc_Node.ln_Name;
            /* Check as long as the names are identical. */
            while (*s1++ == *s2)
                /* Terminator found? */
                if (!*s2++)
                {
                    /* Got it. */
                    ret = thisTask;
                    break;
                }
#endif
        }
    }

#if defined(__AROSEXEC_SMP__)
    EXEC_UNLOCK_AND_ENABLE(listlock);
#else
    Enable();
#endif

    /* Return whatever was found. */
    return ret;

    AROS_LIBFUNC_EXIT
} /* FindTask */

