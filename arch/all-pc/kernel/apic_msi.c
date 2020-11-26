/*
    Copyright © 2017-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_debug.h>
#include <kernel_interrupts.h>
#include <kernel_objects.h>

#define D(x)

ULONG core_APIC_AllocMSI(ULONG count)
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    ULONG msiIRQ = (ULONG)-1;
    UWORD startIRQ = (UWORD)-1, cpuIRQ = (UWORD)-1, irq;

    D(
        bug("[APIC:MSI] %s(%u)\n", __func__, count);
        bug("[APIC:MSI] %s: msibase = %u\n", __func__, apicPrivate->msibase);
        bug("[APIC:MSI] %s: msilast = %u\n", __func__, apicPrivate->msilast);
        bug("[APIC:MSI] %s: HW_IRQ_BASE = %u, HW_IRQ_COUNT = %u\n", __func__, HW_IRQ_BASE, HW_IRQ_COUNT);
    )

    if (apicPrivate->msibase)
    {        
        UBYTE first = apicPrivate->msilast;
        if (!first)
            first = apicPrivate->msibase;

        for (irq = first; irq < ((APIC_IRQ_BASE - X86_CPU_EXCEPT_COUNT) + APIC_IRQ_COUNT); irq++)
        {
            D(bug("[APIC:MSI] %s: trying #%u\n", __func__, irq);)
            if (KERNELIRQ_LIST(irq).lh_Type == APICInt_IntrController.ic_Node.ln_Type)
            {
                D(bug("[APIC:MSI] %s:     .. apic IRQ ..\n", __func__);)
                if (startIRQ == (UWORD)-1)
                {
                    startIRQ = irq;
                    D(bug("[APIC:MSI] %s:  startIRQ = %u\n", __func__, startIRQ);)
                }

                if (irq == (startIRQ + count - 1))
                {
                    cpuIRQ = startIRQ;
                    D(bug("[APIC:MSI] %s:  end = %u\n", __func__, HW_IRQ_BASE + irq);)
                    break;
                }
            }
            else
            {
                startIRQ = cpuIRQ = (UWORD)-1;
            }
        }

        if (cpuIRQ != (UWORD)-1)
        {
            D(bug("[APIC:MSI] %s: Enabling IRQ's #%u -> #%u\n", __func__, cpuIRQ, cpuIRQ + count);)

            for (irq = cpuIRQ; irq < (cpuIRQ + count); irq++)
            {
                ictl_enable_irq(irq, KernelBase);
            }
            msiIRQ = cpuIRQ;

            D(bug("[APIC:MSI] %s: New MSI IRQ ID Base = %u, for %d IRQs\n", __func__, msiIRQ, count);)
            apicPrivate->msilast = cpuIRQ + count;
        }
        else
        {
            D(bug("[APIC:MSI] %s: not enough free IRQs\n", __func__);)
            msiIRQ = (ULONG)-1;
        }
    }
    D(bug("[APIC:MSI] %s: returning %d\n", __func__, msiIRQ);)
    return msiIRQ;
}
