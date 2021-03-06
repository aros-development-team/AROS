/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "apic.h"

AROS_LH0(unsigned int, KrnGetCPUCount,
         struct KernelBase *, KernelBase, 40, Kernel)
{
    AROS_LIBFUNC_INIT

    if (!KernelBase->kb_PlatformData)
        return 0;

    if (!KernelBase->kb_PlatformData->kb_APIC)
        return 1;

    return KernelBase->kb_PlatformData->kb_APIC->apic_count;

    AROS_LIBFUNC_EXIT
}
