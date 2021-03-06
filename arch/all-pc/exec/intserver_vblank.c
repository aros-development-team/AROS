/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#define __AROS_KERNEL__

#include "exec_intern.h"

#include "kernel_intern.h"

#include "intservers.h"

/* VBlankServer. The same as general purpose IntServer but also counts task's quantum */
AROS_INTH3(VBlankServer, struct List *, intList, intMask, custom)
{
    AROS_INTFUNC_INIT

    struct KernelBase *KernelBase = __kernelBase;
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData = pdata->kb_APIC;
#if defined(__AROSEXEC_SMP__)
    struct X86SchedulerPrivate  *apicScheduleData;
    tls_t *apicTLS;
#endif
    D(bug("[Exec:X86] %s()\n", __func__));

    /*
     * If the APIC's dont have their own heartbeat timer,
     * First decrease Elapsed time for current task
     */
    if ((!apicData) || (!(apicData->flags & APF_TIMER)))
    {
#if defined(__AROSEXEC_SMP__)
        if (!apicData)
        {
#endif
            UWORD current = SCHEDELAPSED_GET;
            if (current)
                SCHEDELAPSED_SET(--current);

            if (current == 0)
            {
                FLAG_SCHEDQUANTUM_SET;
                FLAG_SCHEDSWITCH_SET;
            }
#if defined(__AROSEXEC_SMP__)
        }
        else
        {
            /* we can only update cpu #0 */
            apicTLS = apicData->cores[0].cpu_TLS;
            if ((apicTLS) && ((apicScheduleData = apicTLS->ScheduleData) != NULL))
            {
                if (!(apicScheduleData->Elapsed) || (--apicScheduleData->Elapsed == 0))
                {
                    __AROS_ATOMIC_OR_L(apicScheduleData->ScheduleFlags, TLSSF_Quantum);
                    __AROS_ATOMIC_OR_L(apicScheduleData->ScheduleFlags, TLSSF_Switch);
                }
            }
        }
#endif
    }

    /* Chain to the generic routine */
    return AROS_INTC3(IntServer, intList, intMask, custom);

    AROS_INTFUNC_EXIT
}
