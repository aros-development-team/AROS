/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id:$
*/

#include "kernel_base.h"
#include "kernel_ipi.h"
#include <kernel_scheduler.h>
#include <kernel_intr.h>

#include <strings.h>
#include <proto/kernel.h>
#include "apic_ia32.h"
#include "apic.h"

#include "kernel_debug.h"

#define D(x)

void core_DoIPI(uint8_t ipi_number, void *cpu_mask, struct KernelBase *KernelBase)
{
    //int cpunum = KrnGetCPUNumber();
    ULONG cmd = (APIC_IRQ_IPI_START + ipi_number) | ICR_INT_ASSERT;
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    IPTR __APICBase = apicPrivate->lapicBase;

    D(bug("[Kernel:IPI] Sending IPI %02d form CPU.%03u to target mask @ 0x%p\n", ipi_number, cpunum, cpu_mask));
    
    if ((cmd & 0xff) <= APIC_IRQ_IPI_END)
    {
        // special case - send IPI to all
        if ((IPTR)cpu_mask == TASKAFFINITY_ANY)
        {
            // Shorthand - all including self
            cmd |= 0x80000;

            D(bug("[Kernel:IPI] waiting for DS bit to be clear\n"));
            while (APIC_REG(__APICBase, APIC_ICRL) & ICR_DS) asm volatile("pause");
            D(bug("[Kernel:IPI] sending IPI cmd %08x\n", cmd));
            APIC_REG(__APICBase, APIC_ICRL) = cmd;
        }
        else
        {
            int i;

            // No shortcut, send IPI to each CPU one after another
            for (i=0; i < apicPrivate->apic_count; i++)
            {
                if (KrnCPUInMask(i, cpu_mask))
                {
                    D(bug("[Kernel:IPI] waiting for DS bit to be clear\n"));
                    while (APIC_REG(__APICBase, APIC_ICRL) & ICR_DS) asm volatile("pause");
                    D(bug("[Kernel:IPI] sending IPI cmd %08x to destination %08x\n", cmd, (apicPrivate->cores[i].cpu_LocalID << 24)));
                    APIC_REG(__APICBase, APIC_ICRH) = (apicPrivate->cores[i].cpu_LocalID << 24);
                    APIC_REG(__APICBase, APIC_ICRL) = cmd;
                }
            }
        }
    }
}

void core_DoCallIPI(struct Hook *hook, void *cpu_mask, struct KernelBase *KernelBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct IPIHook *ipi = NULL;
    struct APICData *apicPrivate = pdata->kb_APIC;

    if (hook && cpu_mask && cpu_mask != (void*)-1)
    {
        /*
            Allocate IPIHook just by removing it form the Free list:
            First Disable() so that we are not interrupted on this CPU core, then use SpinLock to protect us from
            other cores accessing the list.

            If the FreeIPIHooks list is empty, just do busyloop wait - other cores shall free the hook sooner or later
        */
        do 
        {
            Disable();
            KrnSpinLock(&pdata->kb_FreeIPIHooksLock, NULL, SPINLOCKF_WRITE);
            ipi = (struct IPIHook *)REMHEAD(&pdata->kb_FreeIPIHooks);
            KrnSpinUnLock(&pdata->kb_FreeIPIHooksLock);
            Enable();
            if (ipi == NULL)
            {
                D(bug("[Kernel:IPI] %s: Failed to allocate IPIHook entry\n", __func__));
            }
        } while(ipi == NULL);

        /*
            Copy IPI data from struct Hook provided by caller into allocated ipi
        */
        ipi->ih_Hook.h_Entry = hook->h_Entry;
        ipi->ih_Hook.h_SubEntry = hook->h_SubEntry;
        ipi->ih_Hook.h_Data = hook->h_Data;

        /* Copy CPU mask */
        bcopy(cpu_mask, ipi->ih_CPURequested, sizeof(ULONG)*((31 + apicPrivate->apic_count) / 32));

        /*
            Put the IPIHook on the BusyIPIHooks list, so that it gets processed once IPIs are called
        */
        Disable();
        KrnSpinLock(&pdata->kb_BusyIPIHooksLock, NULL, SPINLOCKF_WRITE);
        ipi = (struct IPIHook *)REMHEAD(&pdata->kb_BusyIPIHooks);
        KrnSpinUnLock(&pdata->kb_BusyIPIHooksLock);
        Enable();

        /* Issue IPI_CALL_HOOK to requested CPUs */
        core_DoIPI(IPI_CALL_HOOK, cpu_mask, KernelBase);
    }
}

void core_IPIHandle(struct ExceptionContext *regs, unsigned long ipi_number, struct KernelBase *KernelBase)
{
    //int cpunum = KrnGetCPUNumber();
    IPTR __APICBase = core_APIC_GetBase();
    
    D(bug("[Kernel:IPI] CPU.%03u IPI%02d\n", cpunum, ipi_number));

    switch (ipi_number)
    {
        case IPI_RESCHEDULE:
            APIC_REG(__APICBase, APIC_EOI) = 0;
            // If IPI was called when CPU was in user mode, perform task switch, otherwise
            // set delayed schedule flag
            if (regs->ss != 0)
            {
                if (core_Schedule())
                {
                    cpu_Switch(regs);
                    cpu_Dispatch(regs);
                }
            }
            else
            {
                FLAG_SCHEDSWITCH_SET;
            }
            break;
    }
}