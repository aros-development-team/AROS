/*
    Copyright (C) 2017-2020, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(void, KrnFreeCPUMask,

/*  SYNOPSIS */
        AROS_LHA(void *, mask, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 43, Kernel)

/*  FUNCTION
        Release storage used by a CPU affinity mask.

    INPUTS
        mask - CPU affinity mask returned by KrnAllocCPUMask().

    RESULT
        NULL.

    NOTES
        NULL and the TASKAFFINITY_* sentinels are ignored.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnAllocCPUMask(), KrnGetCPUMask(), KrnCPUInMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return;

    AROS_LIBFUNC_EXIT
}
