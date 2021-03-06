/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Search a semaphore by name
*/

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_debug.h"

/*****************************************************************************

    NAME */

        AROS_LH1(struct SignalSemaphore *, FindSemaphore,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 99, Exec)

/*  FUNCTION
        Find a semaphore with a given name in the system global semaphore list.
        Note that this call doesn't arbitrate for the list - use Forbid() to
        do this yourself.

    INPUTS
        name - Pointer to name.

    RESULT
        Address of semaphore structure found or NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SignalSemaphore *retVal;

    /* Nothing spectacular - just look into the list */
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->SemListSpinLock, NULL, SPINLOCK_MODE_READ);
#endif
    retVal = (struct SignalSemaphore *)FindName(&SysBase->SemaphoreList,name);
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->SemListSpinLock);
#endif
    return retVal;

    AROS_LIBFUNC_EXIT
} /* FindSemaphore */

