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

static void core_IPICallHookHandle(struct ExceptionContext *regs, struct KernelBase *KernelBase);

extern int Kernel_13_KrnIsSuper(void);

/*
 * Run the calls queued for this CPU from a waiting caller. The hooks expect
 * the interrupt context they normally run in - KrnMapGlobal()'s TLB
 * shootdown reloads cr3 - so a caller in user mode enters supervisor state
 * for them; KrnMapGlobal() itself waits for its shootdown in user mode.
 */
static void core_IPIServeOwnQueue(struct KernelBase *KernelBase)
{
    APTR ssp = NULL;

    if (!Kernel_13_KrnIsSuper())
        ssp = SuperState();
    core_IPICallHookHandle(NULL, KernelBase);
    if (ssp)
        UserState(ssp);
}

/* Take an entry off the free list. Entries only come back when their target
   CPUs drain them, and some may be waiting on this CPU, which cannot take the
   IPI if the caller is inside Disable(): run this CPU's own queue while
   waiting. Disable() before the list lock, never Free and Busy at once. */
static struct IPIHook *core_IPIHookAlloc(struct KernelBase *KernelBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct IPIHook *ipi;

    do
    {
        Disable();
        KrnSpinLock(&pdata->kb_FreeIPIHooksLock, NULL, SPINLOCK_MODE_WRITE);
        ipi = (struct IPIHook *)REMHEAD(&pdata->kb_FreeIPIHooks);
        KrnSpinUnLock(&pdata->kb_FreeIPIHooksLock);
        Enable();
        if (ipi == NULL)
        {
            core_IPIServeOwnQueue(KernelBase);
            asm volatile("pause");
        }
    } while (ipi == NULL);

    return ipi;
}

static void core_IPIHookFree(struct IPIHook *ipi, struct KernelBase *KernelBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    Disable();
    KrnSpinLock(&pdata->kb_FreeIPIHooksLock, NULL, SPINLOCK_MODE_WRITE);
    ADDTAIL(&pdata->kb_FreeIPIHooks, ipi);
    KrnSpinUnLock(&pdata->kb_FreeIPIHooksLock);
    Enable();
}

/* Claiming may spin on an empty pool, so exec claims before it takes the
   target's tc_SpinLock and commits under it once tc_State is re-checked; a
   claimed entry is on no list, so a cancel cannot see it. */
struct CallIPIEntry *core_ClaimCallIPI(int cpu)
{
    (void)cpu;
    return (struct CallIPIEntry *)core_IPIHookAlloc(getKernelBase());
}

void core_AbortCallIPI(struct CallIPIEntry *cie, int cpu)
{
    (void)cpu;
    if (cie)
        core_IPIHookFree((struct IPIHook *)cie, getKernelBase());
}

/* Publish a claimed entry to one CPU, asynchronously: the only caller is
   Signal(), which may run from an interrupt. */
void core_CommitCallIPI(struct CallIPIEntry *cie, int cpu, struct Hook *hook, int nargs, IPTR *args)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicPrivate = pdata->kb_APIC;
    struct IPIHook *ipi = (struct IPIHook *)cie;
    ULONG masklongs = (31 + apicPrivate->apic_count) / 32;
    /* no KrnAllocCPUMask() here, Signal() may run in an interrupt */
    ULONG mask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int i;

    if (!ipi)
        return;
    if (nargs > IPI_CALL_HOOK_MAX_ARGS)
        nargs = IPI_CALL_HOOK_MAX_ARGS;

    ipi->ih_Hook.h_Entry = hook->h_Entry;
    ipi->ih_Hook.h_SubEntry = hook->h_SubEntry;
    ipi->ih_Hook.h_Data = hook->h_Data;
    for (i = 0; i < nargs; i++)
        ipi->ih_Args[i] = args[i];
    ipi->ih_Async = 1;

    bzero(ipi->ih_CPUDone, sizeof(ULONG) * masklongs);
    bzero(ipi->ih_CPUClaimed, sizeof(ULONG) * masklongs);
    bzero(ipi->ih_CPURequested, sizeof(ULONG) * masklongs);
    bit_test_and_set_long(ipi->ih_CPURequested, cpu);
    bit_test_and_set_long(mask, cpu);

    Disable();
    KrnSpinLock(&pdata->kb_BusyIPIHooksLock, NULL, SPINLOCK_MODE_WRITE);
    ADDTAIL(&pdata->kb_BusyIPIHooks, ipi);
    KrnSpinUnLock(&pdata->kb_BusyIPIHooksLock);
    Enable();

    core_DoIPI(IPI_CALL_HOOK, mask, KernelBase);
}

/* Drop every queued call matching (hookEntry, args[1]) and wait out any that
   is executing: a claimed entry is running on another CPU right now, so let
   it finish (it marks itself done and frees the entry) and look again. Called
   from RemTask() once the task is terminal, before its memory is freed. */
void core_CancelCallIPIs(APTR hookEntry, IPTR matchArg)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct PlatformData *pdata;
    struct IPIHook *ipi, *n;
    ULONG masklongs, i;
    BOOL running;

    if (!KernelBase)
        return;
    pdata = KernelBase->kb_PlatformData;
    /* smp_Initialize() only sets the hook lists up with a second CPU */
    if (!pdata || !pdata->kb_APIC || pdata->kb_APIC->apic_count < 2)
        return;
    masklongs = (31 + pdata->kb_APIC->apic_count) / 32;

    do
    {
        running = FALSE;

        Disable();
        KrnSpinLock(&pdata->kb_BusyIPIHooksLock, NULL, SPINLOCK_MODE_WRITE);
        ForeachNodeSafe(&pdata->kb_BusyIPIHooks, ipi, n)
        {
            BOOL claimed = FALSE;

            if ((APTR)ipi->ih_Hook.h_Entry != hookEntry || !ipi->ih_Async || ipi->ih_Args[1] != matchArg)
                continue;
            for (i = 0; i < masklongs; i++)
                if (ipi->ih_CPUClaimed[i])
                    claimed = TRUE;
            if (claimed)
            {
                running = TRUE;
                continue;
            }
            REMOVE(ipi);
            KrnSpinLock(&pdata->kb_FreeIPIHooksLock, NULL, SPINLOCK_MODE_WRITE);
            ADDTAIL(&pdata->kb_FreeIPIHooks, ipi);
            KrnSpinUnLock(&pdata->kb_FreeIPIHooksLock);
        }
        KrnSpinUnLock(&pdata->kb_BusyIPIHooksLock);
        Enable();

        if (running)
        {
            core_IPIServeOwnQueue(KernelBase);
            asm volatile("pause");
        }
    } while (running);
}

int core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async, int nargs, IPTR *args, APTR _KB)
{
    struct KernelBase *KernelBase = _KB;
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct IPIHook *ipi = NULL;
    struct APICData *apicPrivate = pdata->kb_APIC;
    int cpunum = KrnGetCPUNumber();
    int ret = FALSE;
    int i;

    D(bug("[Kernel:IPI.CPU.%03u] %s: Hook @ 0x%p, async=%d\n", cpunum, __func__, hook, async));

    if (nargs > IPI_CALL_HOOK_MAX_ARGS)
        return ret;

    if (hook)
    {
        ipi = core_IPIHookAlloc(KernelBase);

        D(bug("[Kernel:IPI.CPU.%03u] %s: Allocated IPIHook %p\n", cpunum, __func__, ipi));
    
        /*
            Copy IPI data from struct Hook provided by caller into allocated ipi
        */
        ipi->ih_Hook.h_Entry = hook->h_Entry;
        ipi->ih_Hook.h_SubEntry = hook->h_SubEntry;
        ipi->ih_Hook.h_Data = hook->h_Data;
        
        /*
            Copy call hook arguments
        */
        for (i=0; i < nargs; i++)
            ipi->ih_Args[i] = args[i];

        if (async)
        {
            ipi->ih_Async = 1;
        }
        else
        {
            ipi->ih_Async = 0;

            KrnSpinInit(&ipi->ih_SyncLock);
            KrnSpinLock(&ipi->ih_SyncLock, NULL, SPINLOCK_MODE_WRITE);
        }

        /* Clear CPU done and claimed masks */
        bzero(ipi->ih_CPUDone, sizeof(ULONG)*((31 + apicPrivate->apic_count) / 32));
        bzero(ipi->ih_CPUClaimed, sizeof(ULONG)*((31 + apicPrivate->apic_count) / 32));

        /* Copy CPU mask */
        if (cpu_mask && cpu_mask != (void*)TASKAFFINITY_ANY && cpu_mask != (void*)TASKAFFINITY_ALL_BUT_SELF)
            bcopy(cpu_mask, ipi->ih_CPURequested, sizeof(ULONG)*((31 + apicPrivate->apic_count) / 32));
        else
        {
            int i;

            bzero(ipi->ih_CPURequested, sizeof(ULONG)*((31 + apicPrivate->apic_count) / 32));

            for (i=0; i < apicPrivate->apic_count; i++)
            {
                if ((cpu_mask == (APTR)TASKAFFINITY_ALL_BUT_SELF) && (i == cpunum))
                    continue;

                bit_test_and_set_long(ipi->ih_CPURequested, i);
            }
        }

        /*
            Put the IPIHook on the BusyIPIHooks list, so that it gets processed once IPIs are called
        */
        Disable();
        KrnSpinLock(&pdata->kb_BusyIPIHooksLock, NULL, SPINLOCK_MODE_WRITE);
        ADDTAIL(&pdata->kb_BusyIPIHooks, ipi);
        KrnSpinUnLock(&pdata->kb_BusyIPIHooksLock);
        Enable();

        D(bug("[Kernel:IPI.CPU.%03u] %s: Sending IPI message\n", cpunum, __func__, ipi));

        ret = TRUE;

        /* Issue IPI_CALL_HOOK to requested CPUs */
        core_DoIPI(IPI_CALL_HOOK, cpu_mask, KernelBase);

        /* If synchronous IPI, wait for completion */
        if (!async)
        {
            D(bug("[Kernel:IPI.CPU.%03u] %s: Synchronous IPI, waiting for completion\n", cpunum, __func__));
            /* The caller may have interrupts off; serve the calls queued for this CPU meanwhile */
            while (!KrnSpinTryLock(&ipi->ih_SyncLock, SPINLOCK_MODE_WRITE))
            {
                core_IPIServeOwnQueue(KernelBase);
                asm volatile("pause");
            }
            KrnSpinUnLock(&ipi->ih_SyncLock);
            D(bug("[Kernel:IPI.CPU.%03u] %s: Synchronous IPI completed\n", cpunum, __func__));

            /* A synchronous entry is returned by its sender, see core_IPICallHookHandle() */
            core_IPIHookFree(ipi, KernelBase);
        }
    }

    return ret;
}

/*
 * Run the calls queued for this CPU. The hook is called with no list lock
 * held: on pc it is usually signal_hook, whose Signal() may queue an IPI of
 * its own through core_DoCallIPI() and the busy-list lock. An entry is
 * claimed under the lock, called unlocked and marked done under the lock
 * afterwards, so it is freed only when every requested CPU is done and
 * never recycled under a running hook; a synchronous entry is handed back
 * to its waiting sender, which frees it once it holds ih_SyncLock.
 */
static void core_IPICallHookHandle(struct ExceptionContext *regs, struct KernelBase *KernelBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicPrivate = pdata->kb_APIC;
    ULONG masklongs = (31 + apicPrivate->apic_count) / 32;
    int cpunum = KrnGetCPUNumber();
    unsigned long flags;

    D(bug("[Kernel:IPI.CPU.%03u] %s\n", cpunum, __func__));

    /* Also called by a waiting sender, so the list locks are taken with
       interrupts off: a nested IPI_CALL_HOOK would spin on them. */
    for (;;)
    {
        struct IPIHook *ipi = NULL, *n = NULL, *found = NULL;
        BOOL complete = FALSE;

        __save_flags(flags);
        __cli();
        KrnSpinLock(&pdata->kb_BusyIPIHooksLock, NULL, SPINLOCK_MODE_WRITE);
        ForeachNodeSafe(&pdata->kb_BusyIPIHooks, ipi, n)
        {
            if (KrnCPUInMask(cpunum, ipi->ih_CPURequested) &&
                !KrnCPUInMask(cpunum, ipi->ih_CPUClaimed))
            {
                bit_test_and_set_long(ipi->ih_CPUClaimed, cpunum);
                found = ipi;
                break;
            }
        }
        KrnSpinUnLock(&pdata->kb_BusyIPIHooksLock);
        __restore_flags(flags);

        if (!found)
            break;

        /*
            In case of asynchronous hook, the object we pass is IPIHook itself. Dangerous, but we needed to copy the
            original Hook since it could have existed on stack of caller...
        */
        D(bug("[Kernel:IPI.CPU.%03u] %s: Calling HOOK Entry %p with Data %p\n", cpunum, __func__,
            found->ih_Hook.h_Entry, found->ih_Async ? (APTR)&found->ih_Hook : found->ih_Hook.h_Data));
        CALLHOOKPKT(&found->ih_Hook, NULL, 0);

        __save_flags(flags);
        __cli();
        KrnSpinLock(&pdata->kb_BusyIPIHooksLock, NULL, SPINLOCK_MODE_WRITE);
        bit_test_and_set_long(found->ih_CPUDone, cpunum);
        if (!memcmp(found->ih_CPUDone, found->ih_CPURequested, sizeof(ULONG) * masklongs))
        {
            D(bug("[Kernel:IPI.CPU.%03u] %s: IPIHook completed. Releasing it\n", cpunum, __func__));
            REMOVE(found);
            complete = TRUE;
        }
        KrnSpinUnLock(&pdata->kb_BusyIPIHooksLock);

        if (complete)
        {
            if (found->ih_Async)
            {
                KrnSpinLock(&pdata->kb_FreeIPIHooksLock, NULL, SPINLOCK_MODE_WRITE);
                ADDTAIL(&pdata->kb_FreeIPIHooks, found);
                KrnSpinUnLock(&pdata->kb_FreeIPIHooksLock);
            }
            else
            {
                D(bug("[Kernel:IPI.CPU.%03u] %s: Releasing sync lock\n", cpunum, __func__));
                KrnSpinUnLock(&found->ih_SyncLock);
            }
        }
        __restore_flags(flags);
    }
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
            APIC_REG(__APICBase, APIC_EOI) = 0;
            // If IPI was called when CPU was in user mode, and not in forbid state,
            // perform task switch - otherwise set delayed schedule flag.
            if (INTR_FROMUSERMODE && (TDNESTCOUNT_GET < 0))
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

        case IPI_CALL_HOOK:
            core_IPICallHookHandle(regs, KernelBase);
            APIC_REG(__APICBase, APIC_EOI) = 0;
            break;

        default:
            /* Nothing to do, but the local APIC still needs its EOI */
            APIC_REG(__APICBase, APIC_EOI) = 0;
            break;
    }

    return TRUE;
}
