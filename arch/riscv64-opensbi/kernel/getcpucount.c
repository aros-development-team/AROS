/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Report the hart count found in the device tree.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include "kernel_intern.h"

AROS_LH0(unsigned int, KrnGetCPUCount,
         struct KernelBase *, KernelBase, 40, Kernel)
{
    AROS_LIBFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    if (pdata)
        return pdata->kb_HartCount;

    return (unsigned int)__ncpus;

    AROS_LIBFUNC_EXIT
}
