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

        AROS_LH1(void, KrnClearCPUMask,

/*  SYNOPSIS */
        AROS_LHA(void *, mask, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 44, Kernel)

/*  FUNCTION
        Clear all CPU selections in an affinity mask.

    INPUTS
        mask - CPU affinity mask allocated by KrnAllocCPUMask().

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetCPUNumber(), KrnAllocCPUMask(), KrnFreeCPUMask(),
        KrnGetCPUMask(), KrnCPUInMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The actual implementation is entirely architecture-specific */
    return;

    AROS_LIBFUNC_EXIT
}
