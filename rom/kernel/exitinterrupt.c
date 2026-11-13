/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(void, KrnExitInterrupt,

/*  SYNOPSIS */
        AROS_LHA(APTR, ctx, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 62, Kernel)

/*  FUNCTION
        Leave an interrupt using an architecture-defined saved context.

    INPUTS
        ctx - Saved interrupt context to restore.

    RESULT

    NOTES
        The architecture-specific implementation owns context restoration.
        The generic fallback performs no operation.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return;

    AROS_LIBFUNC_EXIT
}
