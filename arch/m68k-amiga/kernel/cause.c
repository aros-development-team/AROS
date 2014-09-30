/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* See rom/kernel/cause.c for documentation */

AROS_LH0I(void, KrnCause,
    struct KernelBase *, KernelBase, 3, Kernel)
{
    AROS_LIBFUNC_INIT

    /* Stub function - this is not needed, since the caller
     * (exec/Cause) does not call KrnCause() on the amiga-m68k
     * platform
     */

    AROS_LIBFUNC_EXIT
}
