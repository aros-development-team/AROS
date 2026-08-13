/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: KrnUnmapGlobal() for the aarch64 Raspberry Pi target.
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include "kernel_intern.h"
#include "mmu.h"

#include <proto/kernel.h>

/* See rom/kernel/unmapglobal.c for documentation. */
AROS_LH2I(int, KrnUnmapGlobal,
        AROS_LHA(void *, virt, A0),
        AROS_LHA(uint32_t, length, D0),
        struct KernelBase *, KernelBase, 17, Kernel)
{
    AROS_LIBFUNC_INIT

    return krnMMUUnmap(virt, length);

    AROS_LIBFUNC_EXIT
}
