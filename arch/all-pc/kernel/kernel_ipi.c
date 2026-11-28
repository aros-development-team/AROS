/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <asm/cpu.h>
#include <strings.h>

#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/exec.h>


#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"

#include "kernel_ipi.h"
#include "kernel_globals.h"

#include <kernel_scheduler.h>
#include <kernel_intr.h>

#include "apic_ia32.h"

#define D(x)

void core_DoIPI(uint8_t ipi_number, void *cpu_mask, struct KernelBase *KernelBase)
{
    int cpunum = KrnGetCPUNumber();
    ULONG cmd = APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_IPI_NOP + ipi_number) | ICR_INT_ASSERT;
    struct PlatformData *kernPlatD = (struct PlatformData *)KernelBase->kb_PlatformData;
    struct APICData *apicPrivate = kernPlatD->kb_APIC;
    IPTR __APICBase = apicPrivate->lapicBase;

    D(bug("[Kernel:IPI] Sending IPI %02d from CPU.%03u to target mask @ 0x%p\n", ipi_number, cpunum, cpu_mask));
    
    asm volatile("sfence");

    if ((cmd & 0xff) <= APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_IPI_CAUSE))
    {
        // special case - send IPI to all
        if ((IPTR)cpu_mask == TASKAFFINITY_ANY)
        {
            // Shorthand - all including self
            cmd |= ICR_DSH_ALL;

            D(bug("[Kernel:IPI] waiting for DS bit to be clear\n"));
            while (APIC_REG(__APICBase, APIC_ICRL) & ICR_DS) asm volatile("pause");
            D(bug("[Kernel:IPI] sending IPI cmd %08x\n", cmd));
            APIC_REG(__APICBase, APIC_ICRL) = cmd;
        }
        else if ((IPTR)cpu_mask == TASKAFFINITY_ALL_BUT_SELF)
        {
            // Shorthand - all excluding self
            cmd |= ICR_DSH_ALLBUTSELF;

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
                    if (cpunum == i)
                    {
                        D(bug("[Kernel:IPI] sending IPI cmd %08x to destination %08x (self-ipi)\n", cmd, (apicPrivate->cores[i].cpu_LocalID << 24)));
                        APIC_REG(__APICBase, APIC_ICRL) = cmd | ICR_DSH_SELF;
                    }
                    else
                    {
                        D(bug("[Kernel:IPI] sending IPI cmd %08x to destination %08x\n", cmd, (apicPrivate->cores[i].cpu_LocalID << 24)));
                        APIC_REG(__APICBase, APIC_ICRH) = (apicPrivate->cores[i].cpu_LocalID << 24);
                        APIC_REG(__APICBase, APIC_ICRL) = cmd;
                    }
                }
            }
        }
    }
    else
    {
        D(bug("[Kernel:IPI] unhandled IPI cmd %08x\n", cmd));
    }
}

int Kernel_13_KrnIsSuper();

/*
 * What rom/kernel/kernel_ipicall.c needs from this port. The requests and
 * the call queues themselves are in that file, the same for every port;
 * here is only how a CPU is interrupted.
 */
ULONG krnIPIPortThisCPU(void)
{
    struct KernelBase *KernelBase = getKernelBase();

    return KrnGetCPUNumber();
}

ULONG krnIPIPortIntrDisable(void)
{
    unsigned long flags;

    __save_flags(flags);
    __cli();

    return (ULONG)flags;
}

void krnIPIPortIntrRestore(ULONG flags)
{
    unsigned long f = flags;

    __restore_flags(f);
}

/* Interrupt one CPU, once for each request that has just become pending */
void krnIPIPortRaise(ULONG cpu, ULONG bits)
{
    struct KernelBase *KernelBase = getKernelBase();
    ULONG mask[KRN_IPICALL_MAXCPUS / 32];
    unsigned long flags;
    ULONG i;

    for (i = 0; i < KRN_IPICALL_MAXCPUS / 32; i++)
        mask[i] = 0;
    mask[cpu / 32] = 1UL << (cpu % 32);

    /* The command is two register writes; nothing may come between them */
    __save_flags(flags);
    __cli();
    if (bits & KRN_IPIF_CALL)
        core_DoIPI(IPI_CALL_HOOK, mask, KernelBase);
    if (bits & KRN_IPIF_RESCHED)
        core_DoIPI(IPI_RESCHEDULE, mask, KernelBase);
    __restore_flags(flags);
}

/* A CPU with interrupts off cannot take its IPI: a sender that waits runs its own calls */
BOOL krnIPIPortCanServe(void)
{
    unsigned long flags;

    __save_flags(flags);

    return (flags & 0x200) ? FALSE : TRUE;
}

/* The hooks need supervisor state (a TLB shootdown reloads cr3) */
void krnIPIPortServe(void)
{
    unsigned long flags;
    APTR ssp = NULL;

    if (!Kernel_13_KrnIsSuper())
        ssp = SuperState();

    __save_flags(flags);
    __cli();
    krnIPICallDrain();
    __restore_flags(flags);

    if (ssp)
        UserState(ssp);
}

/* The CPUs a mask names, as one bit per CPU; the two special masks are resolved here */
static void core_IPIResolveMask(void *cpu_mask, ULONG *mask, struct KernelBase *KernelBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicPrivate = pdata->kb_APIC;
    int cpunum = KrnGetCPUNumber();
    ULONG i;

    for (i = 0; i < KRN_IPICALL_MAXCPUS / 32; i++)
        mask[i] = 0;

    for (i = 0; (i < apicPrivate->apic_count) && (i < KRN_IPICALL_MAXCPUS); i++)
    {
        if (cpu_mask == (void *)TASKAFFINITY_ALL_BUT_SELF)
        {
            if (i == (ULONG)cpunum)
                continue;
        }
        else if (cpu_mask && (cpu_mask != (void *)TASKAFFINITY_ANY))
        {
            if (!KrnCPUInMask(i, cpu_mask))
                continue;
        }
        mask[i / 32] |= 1UL << (i % 32);
    }
}

/* Ask the CPUs in the mask to look at their run state again. A CPU that has not acted on an earlier request is not interrupted again. */
void core_RescheduleCPUs(void *cpu_mask, struct KernelBase *KernelBase)
{
    ULONG mask[KRN_IPICALL_MAXCPUS / 32];
    ULONG i;

    core_IPIResolveMask(cpu_mask, mask, KernelBase);

    for (i = 0; i < KRN_IPICALL_MAXCPUS; i++)
        if (mask[i / 32] & (1UL << (i % 32)))
            krnIPIPost(i, KRN_IPIF_RESCHED);
}

/* Drop the queued calls of hookEntry whose second argument is matchArg, and wait out one that is running */
void core_CancelCallIPIs(APTR hookEntry, IPTR matchArg)
{
    krnIPICallCancel(hookEntry, matchArg);
}

int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async, int nargs, IPTR *args, APTR _KB)
{
    struct KernelBase *KernelBase = _KB;
    ULONG mask[KRN_IPICALL_MAXCPUS / 32];

    if (!hook || (nargs > IPI_CALL_HOOK_MAX_ARGS))
        return FALSE;

    core_IPIResolveMask(cpu_mask, mask, KernelBase);
    krnIPICall(hook, mask, KRN_IPICALL_MAXCPUS / 32, async ? TRUE : FALSE, nargs, args);

    return TRUE;
}

int core_IPIHandle(struct ExceptionContext *regs, void *data1, struct KernelBase *KernelBase)
{
    D(int cpunum = KrnGetCPUNumber();)
    IPTR ipi_number = (IPTR)data1;
    IPTR __APICBase = core_APIC_GetBase();
    
    D(bug("[Kernel:IPI.CPU.%03u] %s: IPI%02d\n", cpunum, __func__, ipi_number));

    switch (ipi_number)
    {
        case IPI_RESCHEDULE:
            /* The request is taken; another one from here on interrupts again */
            krnIPIAck(KRN_IPIF_RESCHED);
            APIC_REG(__APICBase, APIC_EOI) = 0;
            /*
             * Ask for the switch; it is made on the way out. If the CPU was
             * in user mode core_IRQHandle() goes through core_ExitInterrupt()
             * next, which switches unless the task has forbidden it;
             * otherwise the request stays for the next way out. Switching
             * here instead would leave without passing core_ExitInterrupt(),
             * and with that without running the software interrupts another
             * CPU's Cause() sent this request for.
             */
            FLAG_SCHEDSWITCH_SET;
            break;

        case IPI_CALL_HOOK:
            krnIPIAck(KRN_IPIF_CALL);
            krnIPICallDrain();
            APIC_REG(__APICBase, APIC_EOI) = 0;
            break;

        default:
            /* Nothing to do, but the local APIC still needs its EOI */
            APIC_REG(__APICBase, APIC_EOI) = 0;
            break;
    }

    return TRUE;
}
