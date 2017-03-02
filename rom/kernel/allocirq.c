/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(ULONG, KrnAllocIRQ,

/*  SYNOPSIS */
	AROS_LHA(ULONG, irq_type, D0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 38, Kernel)

/*  FUNCTION
	Allocate an arch specific IRQ type.

    INPUTS
	irq_tpe - The Arch specific Type of IRQ to allocate.

    RESULT
	-1 on failure, or the 32bit IRQ ID.

    NOTES

    EXAMPLE
        ULONG msiIRQ = KrnAllocIRQ(IRQTYPE_MSI);

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
