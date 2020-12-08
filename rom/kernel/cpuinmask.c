/*
    Copyright © 2017-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(BOOL, KrnCPUInMask,

/*  SYNOPSIS */
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(void *, mask, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 46, Kernel)

/*  FUNCTION
        Test if the CPU number is enabled in the mask.

    INPUTS
        CPU number (as returned by KrnGetCPUNumber())
        Affinity Mask (as returned by KrnAllocCPUMask())

    RESULT
        TRUE or FALSE if the CPU exists in the mask.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetCPUNumber(), KrnAllocCPUMask(), KrnClearCPUMask(), KrnGetCPUMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The actual implementation is entirely architecture-specific */
    return TRUE;

    AROS_LIBFUNC_EXIT
}
