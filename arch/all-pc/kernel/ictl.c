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
    struct KernelInt *irqInt = &KernelBase->kb_Interrupts[irq];

    D(bug("[Kernel] %s(%d)\n", __func__, irq));

    if ((irqIC = krnGetInterruptController(KernelBase, irqInt->ki_List.lh_Type)) != NULL)
    {
        if ((irqIC->ic_IntrEnable) && (irqIC->ic_IntrEnable(irqIC->ic_Private, irqInt->ki_List.l_pad, irq)))
        {
            D(bug("[Kernel] %s: controller enabled\n", __func__));
            irqInt->ki_Priv |= IRQINTF_ENABLED;
        }
    }
    else
    {
        D(bug("[Kernel] %s: enabled\n", __func__));
        irqInt->ki_Priv |= IRQINTF_ENABLED;
    }
}

void ictl_disable_irq(unsigned char irq, struct KernelBase *KernelBase)
{
    struct IntrController *irqIC;
    struct KernelInt *irqInt = &KernelBase->kb_Interrupts[irq];

    D(bug("[Kernel] %s(%d)\n", __func__, irq));

    if ((irqIC = krnGetInterruptController(KernelBase, irqInt->ki_List.lh_Type)) != NULL)
    {
        if ((irqIC->ic_IntrDisable) && (irqIC->ic_IntrDisable(irqIC->ic_Private, irqInt->ki_List.l_pad, irq)))
        {
            D(bug("[Kernel] %s: controller disabled\n", __func__));
            irqInt->ki_Priv &= ~IRQINTF_ENABLED;
        }
    }
    else
    {
        D(bug("[Kernel] %s: disabled\n", __func__));
        irqInt->ki_Priv &= ~IRQINTF_ENABLED;
    }
}

BOOL ictl_is_irq_enabled(unsigned char irq, struct KernelBase *KernelBase)
{
    struct KernelInt *irqInt = &KernelBase->kb_Interrupts[irq];

    D(bug("[Kernel] %s(%d)\n", __func__, irq));

    return (BOOL)(irqInt->ki_Priv & IRQINTF_ENABLED);
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

#if (__WORDSIZE==64)
    if (!pdata->kb_APIC)
    {
    	/* We are x86-64, so we should always have APIC. */
    	krnPanic(KernelBase, "Failed to allocate APIC descriptor.");
    }
#endif

    /* Check if the 8259a has already been registered, if not probe for it ... */
    if (!krnFindInterruptController(KernelBase, ICTYPE_I8259A))
    {
        D(__unused icintrid_t xtpicICInstID;)

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

    krnPanic(KernelBase, "Failed to detect any Interrupt Controllers.");
}
