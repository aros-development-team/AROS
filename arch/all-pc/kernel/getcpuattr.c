/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include "kernel_intern.h"
#include "apic.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(intptr_t, KrnGetCPUAttr,

/*  SYNOPSIS */
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(uint32_t, cpu, D1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 71, Kernel)

/*  FUNCTION
        Get the value of a per-CPU attribute, for the given CPU.

        See rom/kernel/getcpuattr.c for documentation.

    INPUTS
        id  - ID of the attribute to get
        cpu - CPU number, 0 .. KrnGetCPUCount() - 1

    RESULT
        Value of the attribute, or -1 when the attribute (or the CPU) is
        not known.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetSystemAttr()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData = pdata ? pdata->kb_APIC : NULL;
    struct CPUData *core;

    if (!apicData || cpu >= apicData->apic_count)
        return -1;

    core = &apicData->cores[cpu];

    switch (id)
    {
    case KATTR_CPULoad:
        return core->cpu_Load;

    case KATTR_CPUFrequencyKHz:
        {
            UQUAD khz = core->cpu_TSCFreq / 1000;

            /* Scale the base clock by the governor's current ratio when
               the core runs one */
            if (core->cpu_PerfCapable && core->cpu_PerfMaxRatio)
                khz = khz * core->cpu_PerfCurRatio / core->cpu_PerfMaxRatio;

            return (intptr_t)khz;
        }
    }

    return -1;

    AROS_LIBFUNC_EXIT
}
