/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Enable() - Allow interrupts to occur after Disable().
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <proto/kernel.h>

#include "exec_intern.h"

/*****************************************************************************/
#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

/*  NAME */
#include <proto/exec.h>

        AROS_LH0(void, Enable,

/*  LOCATION */
        struct ExecBase *, SysBase, 21, Exec)

/*  FUNCTION
        This function will allow interrupts to occur after they have
        been disabled by Disable().

        Note that calls to Disable() nest, and for every call to
        Disable() you need a matching call to Enable().

        ***** WARNING *****

        Using this function is considered very harmful, and it is
        not recommended to use this function for ** ANY ** reason.

        It is quite possible to either crash the system, or to prevent
        normal activities (disk/port i/o) from occuring.

        Note: As taskswitching is driven by the interrupts subsystem,
              this function has the side effect of disabling
              multitasking.

    INPUTS
        None.

    RESULT
        Interrupts will be enabled again when this call returns.

    NOTES
        This function preserves all registers.

        To prevent deadlocks calling Wait() in disabled state breaks
        the disable - thus interrupts may happen again.

    EXAMPLE
        No you DEFINITATELY don't want to use this function.

    BUGS
        The only architecture that you can rely on the registers being
        saved is on the Motorola mc68000 family.

    SEE ALSO
        Forbid(), Permit(), Disable(), Wait()

    INTERNALS

******************************************************************************/
{
#undef Exec

    AROS_LIBFUNC_INIT

#ifdef AROS_NO_ATOMIC_OPERATIONS
    SysBase->IDNestCnt--;
#else
    AROS_ATOMIC_DEC(SysBase->IDNestCnt);
#endif

    if (KernelBase && (SysBase->IDNestCnt < 0))
    {
        D(bug("[Enable] Enabling interrupts\n"));
        KrnSti();

        if (KrnIsSuper())
        {
            /* The following stuff is not safe to call from within supervisor mode */
            return;
        }

        /*
         * There's no dff09c like thing in x86 native which would allow
         * us to set delayed (mark it as pending but it gets triggered
         * only once interrupts are enabled again) software interrupt,
         * so we check it manually here in Enable(), similar to Permit().
         */
        if (SysBase->SysFlags & SFF_SoftInt)
        {
            /*
             * First we react on SFF_SoftInt by issuing KrnCause() call. This triggers
             * the complete interrupt processing code in kernel, which implies also
             * rescheduling if became necessary.
             */
            KrnCause();
        }

        if ((SysBase->TDNestCnt < 0) && (SysBase->AttnResched & ARF_AttnSwitch))
        {
            /*
             * If SFF_SoftInt hasn't been set, we have a chance that task switching
             * is enabled and pending. We need to trigger it here in such a case.
             */
            KrnSchedule();        
        }
    }

    AROS_LIBFUNC_EXIT
} /* Enable() */
