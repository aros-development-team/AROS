/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>
#include <exec/tasks.h>

#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(struct Task *, FindTaskByPID,

/*  SYNOPSIS */
	AROS_LHA(ULONG, id, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 166, Exec)

/*  FUNCTION
	Scan through the task lists searching for the task whose
	et_UniqueID field matches.

    INPUTS
	id	-   The task ID to match.

    RESULT
	Address of the Task control structure that matches, or
	NULL otherwise.

    NOTES
    	This function is source-compatible with MorphOS.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    spinlock_t *listLock;
#endif
    struct Task *t;
    struct ETask *et;

    /* First up, check running task(s) */
#if defined(__AROSEXEC_SMP__)
    listLock = EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock, SPINLOCK_MODE_READ);
#endif
    Disable();
#if defined(__AROSEXEC_SMP__)
    ForeachNode(&PrivExecBase(SysBase)->TaskRunning, t)
    {
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
        {
            EXEC_SPINLOCK_UNLOCK(listLock);
            Enable();
	    return t;
        }
    }
    EXEC_SPINLOCK_UNLOCK(listLock);
    Enable();
    listLock = EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskReadySpinLock, SPINLOCK_MODE_READ);
    Disable();
#else
    if (GET_THIS_TASK != NULL)
    {
	et = GetETask(GET_THIS_TASK);
	if (et != NULL && et->et_UniqueID == id)
        {
            Enable();
	    return GET_THIS_TASK;
        }
    }
#endif
    /*	Next, go through the ready list */
    ForeachNode(&SysBase->TaskReady, t)
    {
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
        {
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_UNLOCK(listLock);
#endif
            Enable();
	    return t;
        }
    }
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(listLock);
    Enable();
    listLock = EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskWaitSpinLock, SPINLOCK_MODE_READ);
    Disable();
#endif
    /* Finally, go through the wait list */
    ForeachNode(&SysBase->TaskWait, t)
    {
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
        {
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_UNLOCK(listLock);
#endif
            Enable();
	    return t;
        }
    }

#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(listLock);
#endif
    Enable();

    return NULL;

    AROS_LIBFUNC_EXIT
}
