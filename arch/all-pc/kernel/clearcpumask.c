/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH1(void, KrnClearCPUMask,
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 44, Kernel)
{
    AROS_LIBFUNC_INIT

    ULONG *apicMask;
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData;
    int count, i;
 
    if ((IPTR)mask == TASKAFFINITY_ANY)
        return;

    if ((pdata) && (pdata->kb_APIC) && ((apicMask = (ULONG *)mask) != NULL))
    {
        apicData = pdata->kb_APIC;

        count = apicData->apic_count / 32;

        if ((count * 32) < apicData->apic_count)
            count += 1;

        for (i = 0; i < count; i++)
            apicMask[i] = 0;
    }
    return;

    AROS_LIBFUNC_EXIT
}
