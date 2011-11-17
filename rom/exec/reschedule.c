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

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH0(void, Reschedule,

/*  SYNOPSIS */

/*  LOCATION */
        struct ExecBase *, SysBase, 8, Exec)

/*  FUNCTION
        Give up the CPU time to other tasks (if there are any).

    INPUTS
        None

    RESULT
        None

    NOTES
        This function was private in AmigaOS(tm) up to v3.1. There's no guarantee
        that it will continue to exist in other systems.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_ATOMIC_OR(SysBase->AttnResched, ARF_AttnSwitch);       /* Set scheduling attention */

    if (SysBase->TDNestCnt < 0)                 /* If task switching enabled */
    {
        if (SysBase->IDNestCnt < 0)             /* And interrupts enabled */
        {
            D(bug("[Reschedule] Calling scheduler, KernelBase 0x%p\n", KernelBase));
            KrnSchedule();                      /* Call scheduler */
        }
        /*
         * FIXME: On m68k-amiga we should request software interrupt here.
         * Next Enable() will actually enable interrupts, and the interrupt should
         * happen instantly, with no delay. kernel.resource will see ARF_AttnSwitch
         * flag and perform rescheduling.
         *
         * On other machines we emulate this explicitly in our Enable() function.
         * There we will check AttnResched value we just set.
         *
        else
        {

        } */
    }

    AROS_LIBFUNC_EXIT
} /* Reschedule */
