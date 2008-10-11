/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <proto/kernel.h>

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

    UBYTE flag = SysBase->AttnResched;          /* Save state of scheduling attention */
    
    AROS_ATOMIC_OR(SysBase->AttnResched, 0x80);/* Set scheduling attention */
    
    if (SysBase->TDNestCnt < 0)                 /* If task switching enabled */
    {
        if (SysBase->IDNestCnt < 0)             /* And interrupts enabled */
        {
            KrnSchedule();
        }
        else if (!(flag & 0x80))                /* Generate software interrupt */
        {
#if 0
#warning: "This is wrong!!!!!!!! It should cause the software interrupt only once interrupts are enabled again"
#warning  "but this jumps directly to software interrupt, no matter if interrupts are enabled or not"
            __asm__ ("movl $-3,%eax\n\tint $0x80");
#endif
        }
    }

    AROS_LIBFUNC_EXIT
} /* Reschedule */
