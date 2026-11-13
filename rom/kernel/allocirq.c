/*
    Copyright (C) 2017-2020, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(ULONG, KrnAllocIRQ,

/*  SYNOPSIS */
        AROS_LHA(ULONG, irq_type, D0),
        AROS_LHA(ULONG, count, D1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 38, Kernel)

/*  FUNCTION
        Allocate a consecutive range of interrupt identifiers.

    INPUTS
        irq_type - Platform-defined interrupt type to allocate.
        count    - Number of consecutive interrupt identifiers.

    RESULT
        First allocated interrupt identifier, or (ULONG)-1 on failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnFreeIRQ(), KrnAddIRQHandler()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return (ULONG)-1;

    AROS_LIBFUNC_EXIT
}
