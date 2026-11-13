/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1I(void, KrnSetScheduler,

/*  SYNOPSIS */
        AROS_LHA(KRN_SchedType, sched, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 2, Kernel)

/*  FUNCTION
        Currently leaves the kernel scheduling policy unchanged.

    INPUTS
        sched - Requested scheduling policy (currently ignored).

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetScheduler()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}
