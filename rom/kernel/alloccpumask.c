/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.

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
        Allocate a zero-initialized CPU affinity mask.

    INPUTS

    RESULT
        Pointer to the allocated mask, or NULL if allocation fails (or the
        platform has no CPU masks, as on uniprocessor builds).

    NOTES
        Masks are a fixed size for the lifetime of the boot, large enough
        for every CPU the platform can number, so a mask stays valid as
        CPUs come online.
        A NULL mask is not valid: no CPU is ever selected by it. The
        TASKAFFINITY_ANY and TASKAFFINITY_ALL_BUT_SELF values are sentinels
        rather than mask storage.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnFreeCPUMask(), KrnGetCPUNumber(), KrnClearCPUMask(),
        KrnGetCPUMask(), KrnCPUInMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The actual implementation is entirely architecture-specific */
    return NULL;

    AROS_LIBFUNC_EXIT
}
