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

    /* cpu is the logical CPU number (KrnGetCPUNumber()), boot hart = 0 */
    if (cpu >= __riscv_cpu_count)
        return -1;

    switch (id)
    {
    case KATTR_CPULoad:
        if (pdata && (pdata->kb_CPUHart[cpu] >= 0))
        {
            struct HartData *hd = &pdata->kb_Harts[pdata->kb_CPUHart[cpu]];

            return (hd->hd_Flags & HARTF_ONLINE) ? (intptr_t)hd->hd_Load : 0;
        }
        /* Before the platform data exists only the boot hart runs */
        return (cpu == 0) ? (intptr_t)__cpu_load : 0;

    case KATTR_CPUHardwareID:
        /* The SBI hart id behind a logical CPU number */
        if (pdata && (pdata->kb_CPUHart[cpu] >= 0))
            return (intptr_t)pdata->kb_Harts[pdata->kb_CPUHart[cpu]].hd_HartID;
        return (cpu == 0) ? (intptr_t)__boot_hartid : -1;
    }

    return -1;

    AROS_LIBFUNC_EXIT
}
