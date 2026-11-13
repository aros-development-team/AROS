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

        AROS_LH2(BOOL, KrnCPUInMask,

/*  SYNOPSIS */
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(void *, mask, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 46, Kernel)

/*  FUNCTION
        Determine whether a CPU is selected in an affinity mask.

    INPUTS
        id   - CPU number to test.
        mask - CPU affinity mask allocated by KrnAllocCPUMask().

    RESULT
        TRUE if the CPU is selected; FALSE otherwise. TRUE for any CPU
        with TASKAFFINITY_ANY, and for any CPU but the caller's with
        TASKAFFINITY_ALL_BUT_SELF.

    NOTES
        A NULL mask is not valid and selects no CPU, so FALSE is returned
        for it on SMP platforms. This generic uniprocessor version, where
        KrnAllocCPUMask() returns NULL and affinity is never in force,
        returns TRUE.

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
