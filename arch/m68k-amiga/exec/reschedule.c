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

#include <hardware/custom.h>

#include "exec_intern.h"

/* Amiga(tm) custom chips MMIO area */
#define _CUSTOM 0xDFF000

extern VOID AROS_SLIB_ENTRY(Schedule, Exec, 7)(VOID);

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

    BOOL oldAttnSwitch = (SysBase->AttnResched & ARF_AttnSwitch) ? TRUE : FALSE;

    AROS_ATOMIC_OR(SysBase->AttnResched, ARF_AttnSwitch);       /* Set scheduling attention */

    if (SysBase->TDNestCnt < 0)                 /* If task switching enabled */
    {
        if (SysBase->IDNestCnt < 0)             /* And interrupts enabled */
        {
            D(bug("[Reschedule] Calling scheduler, KernelBase 0x%p\n", KernelBase));
            Supervisor(AROS_SLIB_ENTRY(Schedule,Exec,7));    /* Call scheduler */
        }

        /* If we set the attention flag, tag the software interrupt*/
        if (!oldAttnSwitch) {
            struct Custom *custom = (APTR)_CUSTOM;

            custom->intreq = 0x8004;
        }
    }

    AROS_LIBFUNC_EXIT
} /* Reschedule */
