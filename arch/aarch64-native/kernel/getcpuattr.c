/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: KrnGetCPUAttr() - AArch64.

    The documented per-CPU form of the attributes. The load itself is
    computed by KrnGetSystemAttr(KATTR_CPULoad + n), the older form,
    which stays for compatibility.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include "kernel_intern.h"
#include "kernel_cpu.h"

#include <proto/kernel.h>

/* See rom/kernel/getcpuattr.c for documentation */

AROS_LH2(intptr_t, KrnGetCPUAttr,
         AROS_LHA(uint32_t, id, D0),
         AROS_LHA(uint32_t, cpu, D1),
         struct KernelBase *, KernelBase, 71, Kernel)
{
    AROS_LIBFUNC_INIT

    if (cpu >= KrnGetCPUCount())
        return -1;

    switch (id)
    {
    case KATTR_CPULoad:
        if (cpu < (KATTR_CPULoad_END - KATTR_CPULoad))
            return KrnGetSystemAttr(KATTR_CPULoad + cpu);
        break;

    case KATTR_CPUHardwareID:
        if (cpu < AARCH64_MAXCPUS)
            return (intptr_t)aarch64_CPUHwID[cpu];
        break;
    }

    return -1;

    AROS_LIBFUNC_EXIT
}
