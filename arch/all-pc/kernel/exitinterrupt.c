/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_intr.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(void, KrnExitInterrupt,

/*  SYNOPSIS */
        AROS_LHA(APTR, ctx, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 62, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    core_LeaveInterrupt(ctx);

    AROS_LIBFUNC_EXIT
}
