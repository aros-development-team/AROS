/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Search a task by name.
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"

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
    spinlock_t *listLock;
#endif
    struct Task *ret;

    /* Quick return for a quick argument */
    if (name == NULL)
	return GET_THIS_TASK;

    /* Always protect task lists */
#if defined(__AROSEXEC_SMP__)
    listLock = EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock, SPINLOCK_MODE_READ);
    Forbid();
	/* Then into the waiting list. */
    ret = (struct Task *)FindName(&PrivExecBase(SysBase)->TaskRunning, name);
    if (ret == NULL)
    {
        EXEC_SPINLOCK_UNLOCK(listLock);
        Permit();
        listLock = EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskReadySpinLock, SPINLOCK_MODE_READ);
        Forbid();
#else
    Disable();
#endif

    /* First look into the ready list. */
    ret = (struct Task *)FindName(&SysBase->TaskReady, name);
    if (ret == NULL)
    {
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_UNLOCK(listLock);
        Permit();
        listLock = EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskWaitSpinLock, SPINLOCK_MODE_READ);
        Forbid();
#endif
	/* Then into the waiting list. */
	ret = (struct Task *)FindName(&SysBase->TaskWait, name);
	if (ret == NULL)
	{
	    /*
		Finally test the running task(s). Note that generally
		you know the name of your own task - so it is close
		to nonsense to look for it this way.
	    */
	    char *s1;
	    const char *s2 = name;

#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_UNLOCK(listLock);
            Permit();
            listLock = EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock, SPINLOCK_MODE_READ);
            Forbid();
            ForeachNode(&PrivExecBase(SysBase)->TaskRunning, ret)
            {
                s1 = ret->tc_Node.ln_Name;
#else
            s1 = GET_THIS_TASK->tc_Node.ln_Name;
#endif
	    /* Check as long as the names are identical. */
	    while (*s1++ == *s2)
		/* Terminator found? */
		if (!*s2++)
		{
		    /* Got it. */
#if defined(__AROSEXEC_SMP__)
#else
		    ret = GET_THIS_TASK;
#endif
		    break;
		}
#if defined(__AROSEXEC_SMP__)
            }
#endif
        }
    }

#if defined(__AROSEXEC_SMP__)
    }
    EXEC_SPINLOCK_UNLOCK(listLock);
    Permit();
#else
    Enable();
#endif

    /* Return whatever was found. */
    return ret;

    AROS_LIBFUNC_EXIT
} /* FindTask */

