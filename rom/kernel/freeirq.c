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

        AROS_LH2(void, KrnFreeIRQ,

/*  SYNOPSIS */
        AROS_LHA(ULONG, start, D0),
        AROS_LHA(ULONG, count, D1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 55, Kernel)

/*  FUNCTION
        Release allocated arch specific IRQ's.

    INPUTS
        start - First IRQ ID.
        count   - The number of sequential IRQ's to release.

    RESULT

    NOTES

    EXAMPLE
        ULONG msiIRQBase = KrnAllocIRQ(IRQTYPE_MSI, 7);
        KrnFreeIRQ(msiIRQBase, 7);

    BUGS

    SEE ALSO
        KrnAllocIRQ();

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */

    AROS_LIBFUNC_EXIT
}
