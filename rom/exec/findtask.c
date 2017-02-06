/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Search a task by name.
    Lang: english
*/

#define DEBUG 0

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
    listLock = EXECTASK_SPINLOCK_LOCKFORBID(&PrivExecBase(SysBase)->TaskReadySpinLock, SPINLOCK_MODE_READ);
#else
    Disable();
#endif

    /* First look into the ready list. */
    ret = (struct Task *)FindName(&SysBase->TaskReady, name);
    if (ret == NULL)
    {
#if defined(__AROSEXEC_SMP__)
        EXECTASK_SPINLOCK_UNLOCK(listLock);
        Permit();
        listLock = EXECTASK_SPINLOCK_LOCKFORBID(&PrivExecBase(SysBase)->TaskWaitSpinLock, SPINLOCK_MODE_READ);
#endif
	/* Then into the waiting list. */
	ret = (struct Task *)FindName(&SysBase->TaskWait, name);
	if (ret == NULL)
	{
	    /*
		Finally test the running task(s). This is mainly of importance on smp systems.
	    */
#if defined(__AROSEXEC_SMP__)
            EXECTASK_SPINLOCK_UNLOCK(listLock);
            Permit();
            listLock = EXECTASK_SPINLOCK_LOCKFORBID(&PrivExecBase(SysBase)->TaskRunningSpinLock, SPINLOCK_MODE_READ);
            ret = (struct Task *)FindName(&PrivExecBase(SysBase)->TaskRunning, name);
            if (ret == NULL)
            {
                EXECTASK_SPINLOCK_UNLOCK(listLock);
                Permit();
                listLock = EXECTASK_SPINLOCK_LOCKFORBID(&PrivExecBase(SysBase)->TaskSpinningLock, SPINLOCK_MODE_READ);
                ret = (struct Task *)FindName(&PrivExecBase(SysBase)->TaskSpinning, name);
            }
#else

	    char *s1;
	    const char *s2 = name;
            s1 = GET_THIS_TASK->tc_Node.ln_Name;
	    /* Check as long as the names are identical. */
	    while (*s1++ == *s2)
		/* Terminator found? */
		if (!*s2++)
		{
		    /* Got it. */
		    ret = GET_THIS_TASK;
		    break;
		}
#endif
        }
    }

#if defined(__AROSEXEC_SMP__)
    EXECTASK_SPINLOCK_UNLOCK(listLock);
    Permit();
#else
    Enable();
#endif

    /* Return whatever was found. */
    return ret;

    AROS_LIBFUNC_EXIT
} /* FindTask */

