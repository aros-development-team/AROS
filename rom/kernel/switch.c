/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(void, KrnSwitch,

/*  SYNOPSIS */

/*  LOCATION */
         struct KernelBase *, KernelBase, 5, Kernel)

/*  FUNCTION
        Save context of caller's task and dispatch the next available task

    INPUTS
        None

    RESULT
        None

    NOTES
        This entry point directly calls task switch routine
        in supervisor mode. It neither performs any checks of caller status
        nor obeys interrupt enable state. After calling this function, caller's
        task will be replaced by another one, and it's caller's responsibility
        to do anything to prevent task loss.

        This function is safe to call only from within user mode.
        This function is considered internal, and not meant to be called
        by user's software.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    krnSysCall(SC_SWITCH);

    AROS_LIBFUNC_EXIT
}
