/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: KrnMapGlobal() for the aarch64 Raspberry Pi target.
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include "kernel_intern.h"
#include "mmu.h"

#include <proto/kernel.h>

/* See rom/kernel/mapglobal.c for documentation. */
AROS_LH4I(int, KrnMapGlobal,
        AROS_LHA(void *, virt, A0),
        AROS_LHA(void *, phys, A1),
        AROS_LHA(uint32_t, length, D0),
        AROS_LHA(KRN_MapAttr, flags, D1),
        struct KernelBase *, KernelBase, 16, Kernel)
{
    AROS_LIBFUNC_INIT

    return krnMMUMap(virt, phys, length, flags);

    AROS_LIBFUNC_EXIT
}
