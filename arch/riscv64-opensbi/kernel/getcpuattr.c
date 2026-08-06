/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Per-CPU attributes for the opensbi-riscv64 target.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include "kernel_intern.h"

AROS_LH2(intptr_t, KrnGetCPUAttr,
         AROS_LHA(uint32_t, id, D0),
         AROS_LHA(uint32_t, cpu, D1),
         struct KernelBase *, KernelBase, 71, Kernel)
{
    AROS_LIBFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    if (pdata)
    {
        if (cpu >= pdata->kb_HartCount)
            return -1;

        switch (id)
        {
        case KATTR_CPULoad:
            /* Harts that are not running AROS are truthfully idle */
            return (pdata->kb_Harts[cpu].hd_Flags & HARTF_ONLINE)
                       ? (intptr_t)pdata->kb_Harts[cpu].hd_Load : 0;
        }

        return -1;
    }

    /* Before the platform data exists only the boot hart is described */
    if (cpu >= __ncpus)
        return -1;

    switch (id)
    {
    case KATTR_CPULoad:
        return (cpu == (uint32_t)__boot_cpu_index) ? (intptr_t)__cpu_load : 0;
    }

    return -1;

    AROS_LIBFUNC_EXIT
}
