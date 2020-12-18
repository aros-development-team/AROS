/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_intern.h>
#include <kernel_intr.h>

#include <proto/kernel.h>

AROS_LH1(void, KrnExitInterrupt,
        AROS_LHA(APTR, ctx, A0),
        struct KernelBase *, KernelBase, 62, Kernel)
{
    AROS_LIBFUNC_INIT

    core_LeaveInterrupt(ctx);

    AROS_LIBFUNC_EXIT
}
