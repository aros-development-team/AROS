/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <proto/exec.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH1(void, KrnFreeCPUMask,
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 43, Kernel)
{
    AROS_LIBFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData;
    int count;

    if ((IPTR)mask == TASKAFFINITY_ANY)
        return;

    if ((pdata) && (pdata->kb_APIC))
    {
        apicData  = pdata->kb_APIC;

        count = apicData->apic_count / 32;

        if ((count * 32) < apicData->apic_count)
            count += 1;

        FreeMem(mask, count * sizeof(ULONG));
    }

    return;

    AROS_LIBFUNC_EXIT
}
