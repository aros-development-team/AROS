/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <proto/kernel.h>

#include "exec_intern.h"

/*****i***********************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH1(void, Reschedule,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task, A0),

/*  LOCATION */
        struct ExecBase *, SysBase, 8, Exec)

/*  FUNCTION
        Reschedule will place the task into one of Execs internal task
        lists. Which list it is placed in will depend upon whether the
        task is ready to run, or whether it is waiting for an external
        event to awaken it.

        It is possible that in the future, more efficient schedulers
        will be implemented. In which case this is the function that they
        need to implement.

        You should not do any costly calculations since you will be
        running in interupt mode.

    INPUTS
        task    -   The task to insert into the list.

    RESULT
        The task will be inserted into one of Exec's task lists.

    NOTES
        Not actually complete yet. Some of the task states don't have any
        supplied action.

    EXAMPLE

    BUGS
        Only in relation to the comments within about low-priority tasks
        not getting any processor time.

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_ATOMIC_OR(SysBase->AttnResched, 0x80);	/* Set scheduling attention */

    /*
     * On classic Amiga(tm) hardware software interrupt will be postponed by the
     * chipset if interrupts are disabled, and triggered automatically once interrupts
     * are enabled again.
     * On other machines we emulate this explicitly in our Enable() function.
     * There we will check AttnResched value we just set.
     */
    if (SysBase->TDNestCnt < 0)                 /* If task switching enabled */
    {
        if (SysBase->IDNestCnt < 0)             /* And interrupts enabled */
        {
            D(bug("[Reschedule] Calling scheduler, KernelBase 0x%p\n", KernelBase));
            KrnSchedule();			/* Call scheduler */
        }
    }

    AROS_LIBFUNC_EXIT
} /* Reschedule */
