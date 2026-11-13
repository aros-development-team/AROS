/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include "kernel_ipi.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(void, KrnScheduleCPU,

/*  SYNOPSIS */
        AROS_LHA(void *, cpu_mask, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 47, Kernel)

/*  FUNCTION
        Run task scheduling sequence on all CPUs given in the cpu_mask

    INPUTS
        None

    RESULT
        None

    NOTES
        This entry point directly calls task scheduling routine
        in supervisor mode. It neither performs any checks of caller status
        nor obeys interrupt enable state.

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

    core_DoIPI(IPI_RESCHEDULE, cpu_mask, KernelBase);

    AROS_LIBFUNC_EXIT
}
