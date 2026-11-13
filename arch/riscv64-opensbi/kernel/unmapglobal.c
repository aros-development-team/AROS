/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: KrnUnmapGlobal() for the opensbi-riscv64 target.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include <kernel_base.h>
#include "kernel_intern.h"

#include <proto/kernel.h>

AROS_LH2I(int, KrnUnmapGlobal,
        AROS_LHA(void *, virt, A0),
        AROS_LHA(uint32_t, length, D0),
        struct KernelBase *, KernelBase, 17, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] KrnUnmapGlobal(virt=%p, len=%08x)\n", virt, length));

    return krnMMUUnmapPages((unsigned long)virt, length);

    AROS_LIBFUNC_EXIT
}
