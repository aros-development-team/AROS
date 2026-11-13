/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Forbid() - Prevent tasks switches from taking place.
*/

#define DEBUG 0

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>

#include "exec_intern.h"

#undef Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

/*****************************************************************************/


/*  NAME */
#include <proto/exec.h>

        AROS_LH0(void, Forbid,

/*  LOCATION */
        struct ExecBase *, SysBase, 22, Exec)

/*  FUNCTION
        Forbid any further taskswitches (*) until a matching call to Permit().
        Naturally disabling taskswitches means:

        THIS CALL IS DANGEROUS

        Do not use it without thinking very well about it or better
        do not use it at all. Most of the time you can live without
        it by using semaphores or similar.

        Calls to Forbid() nest, i.e. for each call to Forbid() you
        need one call to Permit().

    INPUTS
        None.

    RESULT
        The multitasking state will be disabled AFTER this function
        returns to the caller.

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
        Permit(), Disable(), Enable(), Wait()

    INTERNALS
        If you want to preserve all the registers, replace this function
        in your $(KERNEL) directory. Otherwise this function is
        satisfactory.

******************************************************************************/
{
#undef Exec

    AROS_LIBFUNC_INIT

    D(bug("[Exec] Forbid()\n"));

    TDNESTCOUNT_INC;

    D(bug("[Exec] Forbid: TDNESTCOUNT = %d\n", TDNESTCOUNT_GET);)

    AROS_LIBFUNC_EXIT
} /* Forbid() */
