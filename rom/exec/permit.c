/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Permit() - Allow tasks switches to occur.
    Lang: english
*/

#define DEBUG 0

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <aros/debug.h>
#include <proto/exec.h>

#include "exec_intern.h"

#undef Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

/*****************************************************************************

    NAME */

        AROS_LH0(void, Permit,

/*  LOCATION */
        struct ExecBase *, SysBase, 23, Exec)

/*  FUNCTION
        This function will reactivate the task dispatcher (*) after a call
        to Forbid(). Note that calls to Forbid() nest, and for every
        call to Forbid() you need a matching call to Permit().

    INPUTS
        None.

    RESULT
        Multitasking will be re-enabled.

    NOTES
        This function preserves all registers.

        To prevent deadlocks calling Wait() in forbidden state breaks
        the forbid - thus taskswitches may happen again.

        (*) On EXECSMP builds, Forbid() only aplies to the processor
            it is called from. Data which needs to be protected from
            parallel access will also require a spinlock.  

    EXAMPLE
        On uniprocessor builds of AROS, it is generally not necessary/
        desirable to use Forbid()/Permit() in most userspace code - however for
        EXECSMP builds, you will need to protect spinlocks against
        task switches on the local processor..

    BUGS
        The only architecture that you can rely on the registers being
        saved is on the Motorola mc68000 family.

    SEE ALSO
        Forbid(), Disable(), Enable(), Wait()

    INTERNALS
        If you want to preserve all the registers, replace this function
        in your $(KERNEL) directory. Otherwise this function is
        satisfactory.

    HISTORY


******************************************************************************/
#undef Exec
{
    AROS_LIBFUNC_INIT

    D(bug("[Exec] Permit()\n"));

    /*
     * Task switches are allowed again, if a switch is pending, we
     * should allow it.
     */
    TDNESTCOUNT_DEC;

    D(bug("[Exec] Permit: TDNESTCOUNT = %d\n", TDNESTCOUNT_GET));

    if (KernelBase && !KrnIsSuper())
    {
        if(    ( TDNESTCOUNT_GET < 0 )
            && ( IDNESTCOUNT_GET < 0 )
            && ( FLAG_SCHEDSWITCH_ISSET ) )
        {
            /*
             * Haha, you re-enabled multitasking, only to have the rug
             * pulled out from under you feet :)

             * Clear the Switch() pending flag.
             */
            D(bug("[Exec] Permit: rescheduling\n"));
            KrnSchedule();
        }
    }

    AROS_LIBFUNC_EXIT
} /* Permit() */
