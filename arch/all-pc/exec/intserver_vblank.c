/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#include "kernel_base.h"
#include "kernel_intern.h"

#define __AROS_KERNEL__

#include "exec_intern.h"

#include "intservers.h"

/* VBlankServer. The same as general purpose IntServer but also counts task's quantum */
AROS_INTH3(VBlankServer, struct List *, intList, intMask, custom)
{
    AROS_INTFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    struct KernelBase *KernelBase = __kernelBase;
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData = pdata->kb_APIC;
    struct X86SchedulerPrivate  *apicScheduleData;
    tls_t *apicTLS;
    apicid_t cpuNo;
#endif

    D(bug("[Exec:X86] %s()\n", __func__));

    /* First decrease Elapsed time for current task */
#if !defined(__AROSEXEC_SMP__)
    if (SCHEDELAPSED_GET && (--SysBase->Elapsed == 0))
    {
        FLAG_SCHEDQUANTUM_SET;
        FLAG_SCHEDSWITCH_SET;
    }
#else
    if (apicData)
    {
        for (cpuNo = 0; cpuNo < apicData->apic_count; cpuNo++)
        {
            apicTLS = apicData->cores[cpuNo].cpu_TLS;
            if ((apicTLS) && ((apicScheduleData = apicTLS->ScheduleData) != NULL))
            {
                if ((apicScheduleData->Elapsed) && (--apicScheduleData->Elapsed == 0))
                {
                    __AROS_ATOMIC_OR_L(apicScheduleData->ScheduleFlags, TLSSF_Quantum);
                    __AROS_ATOMIC_OR_L(apicScheduleData->ScheduleFlags, TLSSF_Switch);
                }
            }
        }
    }
#endif

    /* Chain to the generic routine */
    return AROS_INTC3(IntServer, intList, intMask, custom);

    AROS_INTFUNC_EXIT
}
