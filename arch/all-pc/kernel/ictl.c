/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_interrupts.h"
#include "kernel_debug.h"

#include "apic.h"
#include "i8259a.h"

#define D(x)

void ictl_enable_irq(unsigned char irq, struct KernelBase *KernelBase)
{
    struct IntrController *irqIC;

    D(bug("[Kernel] %s(%d)\n", __func__, irq));

    if ((irqIC = krnGetInterruptController(KernelBase, KernelBase->kb_Interrupts[irq].lh_Type)) != NULL)
    {
        if (irqIC->ic_IntrEnable)
            irqIC->ic_IntrEnable(irqIC->ic_Private, KernelBase->kb_Interrupts[irq].l_pad, irq);
    }
}

void ictl_Initialize(struct KernelBase *KernelBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    int cnt;

    D(bug("[Kernel] %s()\n", __func__));

    if (!pdata->kb_APIC)
    {
	/* No APIC was discovered by ACPI/whatever else. Do the probe. */
	pdata->kb_APIC = core_APIC_Probe();
    }

#if (1)
    if (!pdata->kb_APIC)
    {
    	/* We are x86-64 and we always have APIC. */
    	krnPanic(KernelBase, "Failed to allocate APIC descriptor\n.The system is low on memory.");
    }
#endif

    /* Check if the 8259a has already been registered, if not probe for it ... */
    if (!krnFindInterruptController(KernelBase, ICTYPE_I8259A))
    {
        D(icintrid_t xtpicICInstID;)

        if (i8259a_Probe())
        {
            D(xtpicICInstID =) krnAddInterruptController(KernelBase, &i8259a_IntrController);
        }
    }

    if ((cnt = krnInitInterruptControllers(KernelBase)) > 0)
    {
        D(bug("[Kernel] %s: %d Interrupt Controllers Initialized\n", __func__, cnt));
        return;
    }
    
    // TODO: Panic? No interrupt controlers available...
}
