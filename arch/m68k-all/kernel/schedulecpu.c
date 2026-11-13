/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

AROS_LH1(void, KrnScheduleCPU,
        AROS_LHA(void *, cpu_mask, A0),
        struct KernelBase *, KernelBase, 47, Kernel)
{
    AROS_LIBFUNC_INIT

    /* On m68k, there can be only one!... */
    Supervisor(__AROS_GETVECADDR(SysBase,7));

    AROS_LIBFUNC_EXIT
}
