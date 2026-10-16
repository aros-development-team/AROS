/*
    Copyright (C) 2015-2020, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(void, KrnGetCPUMask,

/*  SYNOPSIS */
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(void *, mask, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 45, Kernel)

/*  FUNCTION
        Add a CPU to an affinity mask.

    INPUTS
        id   - CPU number to select.
        mask - CPU affinity mask allocated by KrnAllocCPUMask().

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetCPUNumber(), KrnAllocCPUMask(), KrnClearCPUMask(), KrnCPUInMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The actual implementation is entirely architecture-specific */
    return;

    AROS_LIBFUNC_EXIT
}
