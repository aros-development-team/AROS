/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved.

    Desc: x86_64 arch specific KrnFmtAlertInfo over-ride
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include "apic.h"

static const char irqstring[] =   "IRQ : #%U - 0x%P";

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(void, KrnFmtAlertInfo,

/*  SYNOPSIS */
        AROS_LHA(STRPTR *, TemplatePtr, A0),
        AROS_LHA(IPTR *, ParamPtr, A1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 65, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (TemplatePtr && ParamPtr) {
        struct PlatformData *pdata;
        IPTR                        *params = ParamPtr;

        if ((pdata = (struct PlatformData *)KernelBase->kb_PlatformData) != NULL)
        {
            /*
             * The state of the CPU this is called on: an alert formatted in
             * supervisor mode is formatted where the interrupt was taken.
             */
            struct CPUData *cpuData = core_APIC_GetCPUData(pdata->kb_APIC);
            ULONG lastState = cpuData ? cpuData->cpu_LastState : pdata->kb_LastState;
            struct IntrNode *lastIntr = cpuData ? cpuData->cpu_LastIntr : pdata->kb_LastIntr;

            if (lastState & (PLATFORMF_INIRQ << 16))
            {
                /* Crashed inside an interrupt handler ... */
                *TemplatePtr = (STRPTR)irqstring;
                params[0] = (IPTR)lastState & 0xFF;
                params[1] = (IPTR)lastIntr;
            }
        }
    }
    return;

    AROS_LIBFUNC_EXIT
}
