/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Interrupt source release.
*/

#include <aros/kernel.h>
#include <aros/irqtypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_intern.h>

#include <proto/kernel.h>

/*****************************************************************************

    NAME */

        AROS_LH2(void, KrnFreeIRQ,

/*  SYNOPSIS */
        AROS_LHA(ULONG, start, D0),
        AROS_LHA(ULONG, count, D1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 55, Kernel)

/*  FUNCTION
        Give back sources taken with KrnAllocIRQ().

    INPUTS
        start - The first source number.
        count - How many were taken.

    RESULT
        None.

    NOTES
        Handlers registered on the sources should be removed first;
        nothing here does it for the caller.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnAllocIRQ()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    krnFreeVirtualIRQ(start, count);

    AROS_LIBFUNC_EXIT
}
