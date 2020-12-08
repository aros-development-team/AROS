/*
    Copyright © 2017-2020, The AROS Development Team. All rights reserved.
    $Id$

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
        Allocate an arch specific IRQ type.

    INPUTS
        irq_tpe - The Arch specific Type of IRQ to allocate.
        count   - The number of sequential IRQ's to allocate.

    RESULT
        -1 on failure, or the first 32bit IRQ ID.

    NOTES

    EXAMPLE
        ULONG msiIRQBase = KrnAllocIRQ(IRQTYPE_MSI, 7);

    BUGS

    SEE ALSO
        KrnAddIRQHandler();

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return (ULONG)-1;

    AROS_LIBFUNC_EXIT
}
