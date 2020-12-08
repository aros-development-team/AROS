/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH0(void *, KrnAllocCPUMask,

/*  SYNOPSIS */

/*  LOCATION */
        struct KernelBase *, KernelBase, 42, Kernel)

/*  FUNCTION
        Allocate storage for a CPU mask

    INPUTS

    RESULT
        CPU affinity mask storage

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetCPUNumber(), KrnClearCPUMask(), KrnGetCPUMask(), KrnCPUInMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The actual implementation is entirely architecture-specific */
    return NULL;

    AROS_LIBFUNC_EXIT
}
