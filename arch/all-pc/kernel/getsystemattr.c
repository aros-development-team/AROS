/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include "kernel_intern.h"


/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(intptr_t, KrnGetSystemAttr,

/*  SYNOPSIS */
        AROS_LHA(uint32_t, id, D0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 29, Kernel)

/*  FUNCTION
        Get value of internal system attributes.
        Currently defined attributes are:

          KATTR_Architecture [.G] (char *) - Name of architecture the kernel was built for.

          KATTR_AffinityMask [.G] IPTR     - Bitmap of CPUs, indexed by CPU number.

          KATTR_CPULoad + n [.G] IPTR      - Load of APIC CPU n when APIC data is available.

          KATTR_ClockSource [.G] APTR      - Active kernel clock source, when present.

          KATTR_SymResolverABI [.G] IPTR   - Symbol-resolver ABI level.

    INPUTS
        id - ID of the attribute to get

    RESULT
        Value of the attribute

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData = pdata ? pdata->kb_APIC : NULL;

    intptr_t retval = -1;

    if ((apicData) && (id >= KATTR_CPULoad && id < KATTR_CPULoad_END))
    {
        id -= KATTR_CPULoad;
        if (id < apicData->apic_count)
            retval = apicData->cores[id].cpu_Load;
    }
    else if (id == KATTR_AffinityMask)
    {
        /*
         * The CPUs as a bitmap of CPU numbers, as on the other SMP
         * ports. IPTR-wide, so it describes at most the first 64.
         */
        int count = apicData ? apicData->apic_count : 1;

        retval = (count >= (int)(sizeof(IPTR) * 8)) ? (intptr_t)-1
                                                    : (intptr_t)((1ULL << count) - 1);
    }
    else if (id == KATTR_Architecture)
    {
        retval = (intptr_t)AROS_ARCHITECTURE;
    }
    else if ((id == KATTR_ClockSource) && (KernelBase->kb_ClockSource))
    {
        retval = (intptr_t)KernelBase->kb_ClockSource;
    }
    else if (id == KATTR_SymResolverABI)
    {
        retval = KRN_SYMRESOLVER_ABI_LEASE;
    }
    
    return retval;

    AROS_LIBFUNC_EXIT
}
