/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.

    Desc: KrnModifyIRQA() - pc.
*/


#include <proto/arossupport.h>
#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_irqaffinity.h>

#include <resources/kernel.h>

#include <aros/debug.h>

#include <proto/kernel.h>

#include "kernel_intern.h"
#include "kernel_ipi.h"
#include "apic.h"
#include "ioapic.h"

/* How often a CPU that still has the interrupt to serve is asked again */
#define IRQMOVE_TRIES   100

/* A core that was never started is counted by KrnGetCPUCount() */
static int pc_IRQCPUUsable(struct KernelBase *KernelBase, unsigned int cpu)
{
#if defined(__AROSEXEC_SMP__)
    return cpu_IsOnline(cpu);
#else
    return (cpu == 0);
#endif
}

/* A local APIC id has eight bits here, so there are no more CPUs than this */
#define PC_IRQ_CPUS     256

/* Which CPU a KERNELTAG_IRQ_AFFINITY request means here, or -1 */
static int pc_IRQAffinityCPU(struct KernelBase *KernelBase, IPTR affinity)
{
    unsigned int given[PC_IRQ_CPUS];
    struct IntrMapping *irqMap;
    unsigned int cpus = KrnGetCPUCount(), cpu;
    IPTR lockState;

    if (cpus > PC_IRQ_CPUS)
        cpus = PC_IRQ_CPUS;
    for (cpu = 0; cpu < cpus; cpu++)
        given[cpu] = 0;

    /*
     * The interrupts each CPU has been given: every interrupt that was
     * sent to a CPU has a mapping that names it.
     */
    lockState = ictl_Lock();
    ForeachNode(&KernelBase->kb_InterruptMappings, irqMap)
    {
        if (irqMap->im_CPU < cpus)
            given[irqMap->im_CPU]++;
    }
    ictl_Unlock(lockState);

    return krnIRQAffinityCPUGiven(KernelBase, affinity, cpus, pc_IRQCPUUsable, given);
}

#if defined(__AROSEXEC_SMP__)
/* Runs on the CPU the interrupt is delivered to: see IOAPIC_MoveIRQ() */
AROS_UFH3(static IPTR, pc_MoveIRQHook,
    AROS_UFHA(struct IPIHook *, hook, A0),
    AROS_UFHA(APTR, object, A2),
    AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT

    struct KernelBase *KernelBase = (struct KernelBase *)hook->ih_Hook.h_Data;
    LONG *result = (LONG *)hook->ih_Args[2];

    *result = IOAPIC_MoveIRQ(KernelBase, (irqid_t)hook->ih_Args[0], (ULONG)hook->ih_Args[1]);

    return 0;

    AROS_USERFUNC_EXIT
}
#endif

/*
 * Have an IOAPIC pin delivered to another CPU. The CPU it is delivered to
 * now does the work, when it has no interrupt of the pin left to serve, so
 * that the handlers are never entered on both. The mapping exists.
 */
static BOOL pc_MoveIRQ(struct KernelBase *KernelBase, ULONG irq, ULONG cpu)
{
#if defined(__AROSEXEC_SMP__)
    struct Hook hook = { { 0 }, (APTR)pc_MoveIRQHook, NULL, KernelBase };
    LONG result = IOAPIC_MOVE_BUSY;
    int tries;

    for (tries = 0; tries < IRQMOVE_TRIES; tries++)
    {
        ULONG mask[KRN_IPICALL_MAXCPUS / 32];
        struct IntrMapping *irqMap;
        IPTR args[3];
        IPTR lockState;
        ULONG cur, i;

        lockState = ictl_Lock();
        irqMap = krnInterruptMapping(KernelBase, irq);
        cur = irqMap ? irqMap->im_CPU : 0;
        ictl_Unlock(lockState);

        if (cur >= KRN_IPICALL_MAXCPUS)
            break;

        for (i = 0; i < KRN_IPICALL_MAXCPUS / 32; i++)
            mask[i] = 0;
        mask[cur / 32] = 1UL << (cur % 32);

        args[0] = irq;
        args[1] = cpu;
        args[2] = (IPTR)&result;

        /*
         * Waits for the hook to have run. Not with the controllers' lock
         * held: the hook takes it, and a CPU that waits with interrupts
         * off enters supervisor mode to run the calls sent to it.
         */
        result = IOAPIC_MOVE_BUSY;
        if (!krnIPICall(&hook, mask, KRN_IPICALL_MAXCPUS / 32, FALSE, 3, args))
        {
            result = IOAPIC_MOVE_FAILED;
            break;
        }

        if ((result == IOAPIC_MOVE_DONE) || (result == IOAPIC_MOVE_FAILED))
            break;
    }

    if (result == IOAPIC_MOVE_DONE)
        return TRUE;

    IOAPIC_MoveIRQEnd(KernelBase, irq);
#endif
    return FALSE;
}

/*****************************************************************************

    NAME */

        AROS_LH2(ULONG, KrnModifyIRQA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, irq, D0),
        AROS_LHA(struct TagItem *, attribs, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 39, Kernel)

/*  FUNCTION
        Change how an interrupt source is treated.

    INPUTS
        irq     - The interrupt to change.
        attribs - KERNELTAG_IRQ_POLARITY     - 0 = default, 1 = high, 2 = low.
                  KERNELTAG_IRQ_TRIGGERLEVEL - 0 = default, 1 = level, 2 = edge.
                  KERNELTAG_IRQ_AFFINITY     - the CPUs the interrupt may be
                      delivered to: TASKAFFINITY_BOOT, TASKAFFINITY_ANY or
                      a CPU mask. One of them is chosen, once, the least
                      loaded at the time, and the interrupt is delivered
                      there from then on, whether or not it is enabled or
                      has a handler yet.

    RESULT
        TRUE when everything asked for was done. FALSE, with nothing
        changed, when it was not: the interrupt does not exist, no tag
        known here was passed, there was no memory, or the affinity could
        not be had:
            - it names no CPU that is running;
            - the interrupt is not on an IOAPIC pin or a local APIC (MSI)
              vector and the CPU chosen is not the boot CPU. The 8259A
              delivers to the boot CPU only;
            - an IOAPIC pin never went quiet on the CPU it is delivered to.

    NOTES
        An interrupt that was never given an affinity is delivered to the
        boot CPU.

        An IOAPIC pin can be moved while its interrupt is being raised.
        The call returns when it is delivered to the new CPU and the old
        one has no interrupt of it left to serve; the handlers are not
        entered on both.

        A message signalled interrupt goes where the message its device
        sends is addressed, and that is written by whoever programs the
        device, not here. For an MSI vector the call records the CPU and
        nothing else, and the interrupt is delivered where it was until
        the message is written again. A PCI device's vectors are moved
        with its SetVectorAttribs method, which makes this call and
        writes the message; a driver does not call this for them itself.

        A handler moved off the boot CPU can run at the same time as the
        task code of its driver, which Disable() no longer keeps apart.

        Must not be called from an interrupt handler.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetIRQA(), KrnAllocIRQ(), KrnAllocCPUMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE irqPol = (UBYTE)-1, irqTrig = (UBYTE)-1;
    struct IntrMapping *irqMap, *newMap = NULL;
    struct IntrController *irqIC;
    struct TagItem *irqTag;
    ULONG icType = (ULONG)-1;
    BOOL setCPU = FALSE, done = TRUE;
    IPTR lockState;
    int irqCPU = 0;

    D(bug("[Kernel] %s(%u, 0x%p)\n", __func__, irq, attribs);)

    if (!attribs || (irq >= HW_IRQ_COUNT))
        return FALSE;

    if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_POLARITY, attribs)))
    {
        irqPol = (UBYTE)irqTag->ti_Data;
    }
    if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_TRIGGERLEVEL, attribs)))
    {
        irqTrig = (UBYTE)irqTag->ti_Data;
    }
    if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_AFFINITY, attribs)))
    {
        if ((irqCPU = pc_IRQAffinityCPU(KernelBase, irqTag->ti_Data)) < 0)
            return FALSE;
        setCPU = TRUE;
    }

    if (!setCPU && (irqPol == (UBYTE)-1) && (irqTrig == (UBYTE)-1))
        return FALSE;

    /* The controller an interrupt belongs to is settled when they are set up */
    if ((irqIC = krnGetInterruptController(KernelBase, KERNELIRQ_ICID(irq))) != NULL)
        icType = irqIC->ic_Type;

    /*
     * Only an IOAPIC pin and a local APIC vector can be sent to a CPU of
     * choice. Everything else is delivered to the boot CPU.
     */
    if (setCPU && (irqCPU != 0) && (icType != IIC_ID_IOAPIC) && (icType != IIC_ID_APIC))
        return FALSE;

    /*
     * The mappings list is read by the controllers on every CPU, at
     * interrupt time too, so it is changed under their lock. Memory is
     * not allocated with that held: look, allocate, look again.
     *
     * An interrupt without a mapping is delivered to the boot CPU, so one
     * that is only to stay there does not need a mapping made for it.
     */
    lockState = ictl_Lock();
    irqMap = krnInterruptMapping(KernelBase, irq);
    ictl_Unlock(lockState);

    if (irqMap == NULL)
    {
        if ((irqPol == (UBYTE)-1) && (irqTrig == (UBYTE)-1) && (irqCPU == 0))
            return TRUE;

        if ((newMap = AllocMem(sizeof(struct IntrMapping), MEMF_CLEAR)) == NULL)
            return FALSE;
    }

    lockState = ictl_Lock();
    if (irqMap == NULL)
        irqMap = krnInterruptMapping(KernelBase, irq);
    if (irqMap == NULL)
    {
        irqMap = newMap;
        irqMap->im_DeviceIRQ = irq;
        irqMap->im_Int = irq;
        /*
         * The boot CPU, not the caller's: device interrupts are
         * delivered there unless something asks for another CPU.
         */
        irqMap->im_CPU = 0;
        Enqueue(&KernelBase->kb_InterruptMappings, &irqMap->im_Node);
        newMap = NULL;
    }

    if (setCPU && (irqMap->im_CPU != (ULONG)irqCPU) && (icType != IIC_ID_IOAPIC))
    {
        /*
         * A local APIC vector: the message is addressed by whoever
         * programs the device, who reads the CPU back from here and
         * writes the message again.
         */
        if (icType == IIC_ID_APIC)
            irqMap->im_CPU = irqCPU;
        else
            done = FALSE;
    }
    ictl_Unlock(lockState);

    /* Not used: another caller mapped the interrupt between the two looks */
    if (newMap)
        FreeMem(newMap, sizeof(struct IntrMapping));

    /* A mapping is not unlinked or freed, so it can be used without the lock */
    if (done && setCPU && (icType == IIC_ID_IOAPIC) && (irqMap->im_CPU != (ULONG)irqCPU))
        done = pc_MoveIRQ(KernelBase, irq, irqCPU);

    if (!done)
        return FALSE;

    if ((irqPol != (UBYTE)-1) || (irqTrig != (UBYTE)-1))
    {
        lockState = ictl_Lock();
        if (irqPol != (UBYTE)-1)
            irqMap->im_Polarity = irqPol;
        if (irqTrig != (UBYTE)-1)
            irqMap->im_Trig = irqTrig;
        D(bug("[Kernel] %s: pol = %u, trig = %u\n", __func__, irqMap->im_Polarity, irqMap->im_Trig);)
        ictl_Unlock(lockState);

        /* If the interupt is already enabled, disable and re-enable to update its state .. */
        if (ictl_is_irq_enabled(irq, KernelBase))
        {
            ictl_disable_irq(irq,KernelBase);
            ictl_enable_irq(irq, KernelBase);
        }
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
}
