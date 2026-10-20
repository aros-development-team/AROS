/*
    Copyright (C) 2022-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH0(UQUAD, KrnTimeStamp,

/*  SYNOPSIS */

/*  LOCATION */
        struct KernelBase *, KernelBase, 64, Kernel)

/*  FUNCTION
        Read the architecture-specific timestamp counter.

    INPUTS

    RESULT
        Architecture-specific timestamp value.

    NOTES
        The timestamp unit and frequency depend on the architecture.
        The generic fallback returns zero. This is not wall-clock time.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The actual implementation is entirely architecture-specific */
    return 0;

    AROS_LIBFUNC_EXIT
}
