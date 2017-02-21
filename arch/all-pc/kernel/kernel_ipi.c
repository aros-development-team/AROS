/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id:$
*/

#include "kernel_base.h"
#include "kernel_ipi.h"
#include <kernel_scheduler.h>
#include <kernel_intr.h>

#include <proto/kernel.h>
#include "apic_ia32.h"

#include "kernel_debug.h"

#define D(x)

void core_DoIPI(uint8_t ipi_number, unsigned int cpu_mask, struct KernelBase *KernelBase)
{
    //int cpunum = KrnGetCPUNumber();
    ULONG cmd = (APIC_IRQ_IPI_START + ipi_number) | ICR_INT_ASSERT;
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    IPTR __APICBase = apicPrivate->lapicBase;

    D(bug("[Kernel:IPI] Sending IPI %02d form CPU.%03u to target mask %08x\n", ipi_number, cpunum, cpu_mask));
    
    if ((cmd & 0xff) <= APIC_IRQ_IPI_END)
    {
        // special case - send IPI to all
        if (cpu_mask == 0xffffffff)
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
                if (cpu_mask & (1 << i))
                {
                    ULONG id = apicPrivate->cores[i].cpu_LocalID;
                    D(bug("[Kernel:IPI] waiting for DS bit to be clear\n"));
                    while (APIC_REG(__APICBase, APIC_ICRL) & ICR_DS) asm volatile("pause");
                    D(bug("[Kernel:IPI] sending IPI cmd %08x to destination %08x\n", cmd, id << 24));
                    APIC_REG(__APICBase, APIC_ICRH) = id << 24;
                    APIC_REG(__APICBase, APIC_ICRL) = cmd;
                }
            }
        }
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