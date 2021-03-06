/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <proto/exec.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0(void *, KrnAllocCPUMask,
        struct KernelBase *, KernelBase, 42, Kernel)
{
    AROS_LIBFUNC_INIT

    void *mask = NULL;
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData;
    int count;

    if ((pdata) && (pdata->kb_APIC))
    {
        apicData  = pdata->kb_APIC;

        count = apicData->apic_count / 32;

        if ((count * 32) < apicData->apic_count)
            count += 1;

        mask = AllocMem(count * sizeof(ULONG), MEMF_CLEAR | MEMF_PUBLIC);
    }

    return mask;

    AROS_LIBFUNC_EXIT
}
