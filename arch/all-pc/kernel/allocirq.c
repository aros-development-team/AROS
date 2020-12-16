/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/kernel.h>

#define D(x)

AROS_LH2(ULONG, KrnAllocIRQ,
	AROS_LHA(ULONG, irq_type, D0),
	AROS_LHA(ULONG, count, D1),
	struct KernelBase *, KernelBase, 38, Kernel)
{
    AROS_LIBFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    ULONG irqID = (ULONG)-1;

    D(bug("[KRN] KrnAllocIRQ(%08x):\n", irq_type));

    if ((irq_type == IRQTYPE_APIC) && (pdata->kb_PDFlags & PLATFORMF_HAVEMSI))
    {
        D(bug("[KRN] KrnAllocIRQ: Attempting to allocate APIC Interrupt...\n"));
        irqID = core_APIC_AllocMSI(count);
    }
    return irqID;

    AROS_LIBFUNC_EXIT
}
