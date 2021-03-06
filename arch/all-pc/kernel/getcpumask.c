/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH2(void, KrnGetCPUMask,
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 45, Kernel)
{
    AROS_LIBFUNC_INIT

    core_APIC_GetMask(KernelBase->kb_PlatformData->kb_APIC, id, (cpumask_t *)mask);

    return;

    AROS_LIBFUNC_EXIT
}
