/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
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

ULONG core_APIC_AllocMSI()
{
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    apicid_t cpuNo = KrnGetCPUNumber();
    apicidt_t *IGATES = (apicidt_t *)apicPrivate->cores[cpuNo].cpu_IDT;
    ULONG msiID = APIC_MSI_BASE;
    int cpuIRQ = 0, irq;

    D(bug("[APIC:MSI] %s()\n"));

    for (irq = (APIC_IRQ_BASE - X86_CPU_EXCEPT_COUNT); irq < ((APIC_IRQ_BASE - X86_CPU_EXCEPT_COUNT) + APIC_IRQ_COUNT); irq++)
    {
        if (KERNELIRQ_LIST(irq).lh_Type == APICInt_IntrController.ic_Node.ln_Type)
        {
            if (!IGATES[HW_IRQ_BASE + irq].p)
            {
                IGATES[HW_IRQ_BASE + irq].p = 1;
                cpuIRQ = HW_IRQ_BASE + irq;
                break;
            }
        }
    }
    if (cpuIRQ != 0)
    {
        msiID += ((cpuNo << 8) | (cpuIRQ));

        D(bug("[APIC:MSI] %s: New MSI IRQ ID = %d\n", __func__, (int)msiID));
    }
    else
        msiID = (ULONG)-1;

    return msiID;
}

void core_APIC_RegisterMSI(void *handle)
{
    struct IntrNode *msihandle = (struct IntrNode *)handle;

    D(bug("[APIC:MSI] %s: MSI Handler @ 0x%p\n", msihandle));
    if ((msihandle->in_nr >= APIC_MSI_BASE) && (msihandle->in_nr < (ULONG)-1))
    {
        ULONG tmp = msihandle->in_nr - APIC_MSI_BASE;
        int msiCPUIRQ = (tmp & 0xFF);
        apicid_t msiCPUNo = (tmp >> 8) & 0xFF;

        bug("[APIC:MSI] %s: Registering MSI %d (%03u:%02X)\n", (int)msihandle->in_nr, msiCPUNo, msiCPUIRQ);
    }
}
