/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

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
#else
    struct Task *thisTask = GET_THIS_TASK;
#endif
    struct Task *t;
    struct ETask *et;

    D(bug("[EXEC] %s()\n", __func__);)

    /* First up, check running task(s) */
#if defined(__AROSEXEC_SMP__)
    listLock = EXECTASK_SPINLOCK_LOCKFORBID(&PrivExecBase(SysBase)->TaskRunningSpinLock, SPINLOCK_MODE_READ);
    ForeachNode(&PrivExecBase(SysBase)->TaskRunning, t)
    {
        D(bug("[EXEC] %s: trying Running Task @ 0x%p\n", __func__, t);)
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
        {
            EXECTASK_SPINLOCK_UNLOCK(listLock);
            Permit();
	    return t;
        }
    }
    EXECTASK_SPINLOCK_UNLOCK(listLock);
    Permit();
    listLock = EXECTASK_SPINLOCK_LOCKFORBID(&PrivExecBase(SysBase)->TaskSpinningLock, SPINLOCK_MODE_READ);
    ForeachNode(&PrivExecBase(SysBase)->TaskSpinning, t)
    {
        D(bug("[EXEC] %s: trying Spinning Task @ 0x%p\n", __func__, t);)
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
        {
            EXECTASK_SPINLOCK_UNLOCK(listLock);
            Permit();
	    return t;
        }
    }
    EXECTASK_SPINLOCK_UNLOCK(listLock);
    Permit();
    listLock = EXECTASK_SPINLOCK_LOCKFORBID(&PrivExecBase(SysBase)->TaskReadySpinLock, SPINLOCK_MODE_READ);
#else
    Disable();
    if ((t = thisTask) != NULL)
    {
        D(bug("[EXEC] %s: trying ThisTask @ 0x%p\n", __func__, t);)
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
        {
            Enable();
	    return t;
        }
    }
#endif
    /*	Next, go through the ready list */
    ForeachNode(&SysBase->TaskReady, t)
    {
        D(bug("[EXEC] %s: trying Ready Task @ 0x%p\n", __func__, t);)
	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
        {
#if defined(__AROSEXEC_SMP__)
            EXECTASK_SPINLOCK_UNLOCK(listLock);
            Permit();
#else
            Enable();
#endif
	    return t;
        }
    }
#if defined(__AROSEXEC_SMP__)
    EXECTASK_SPINLOCK_UNLOCK(listLock);
    Permit();
    listLock = EXECTASK_SPINLOCK_LOCKFORBID(&PrivExecBase(SysBase)->TaskWaitSpinLock, SPINLOCK_MODE_READ);
#endif
    /* Finally, go through the wait list */
    ForeachNode(&SysBase->TaskWait, t)
    {
        D(bug("[EXEC] %s: trying Waiting Task @ 0x%p\n", __func__, t);)

	et = GetETask(t);
	if (et != NULL && et->et_UniqueID == id)
        {
#if defined(__AROSEXEC_SMP__)
            EXECTASK_SPINLOCK_UNLOCK(listLock);
            Permit();
#else
            Enable();
#endif
	    return t;
        }
    }

#if defined(__AROSEXEC_SMP__)
    EXECTASK_SPINLOCK_UNLOCK(listLock);
    Permit();
#else
    Enable();
#endif

    D(bug("[EXEC] %s: Not Found\n", __func__);)

    return NULL;

    AROS_LIBFUNC_EXIT
}
