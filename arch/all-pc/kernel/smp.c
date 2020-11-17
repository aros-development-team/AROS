/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__

#include <aros/types/spinlock_s.h>
#include <aros/atomic.h>
#include <asm/io.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_intern.h"
#include "kernel_syscall.h"
#include "kernel_ipi.h"
#include "smp.h"

#define D(x)
#define DWAKE(x)

extern const void *_binary_smpbootstrap_start;
extern const void *_binary_smpbootstrap_size;

extern APTR PlatformAllocGDT(struct KernelBase *, apicid_t);
extern APTR PlatformAllocTLS(struct KernelBase *, apicid_t);
extern APTR PlatformAllocIDT(struct KernelBase *, apicid_t);

#if defined(__AROSEXEC_SMP__)
extern void cpu_PrepareExec(struct ExecBase *);
extern struct Task *cpu_InitBootStrap(apicid_t);
extern void cpu_BootStrap(struct Task *);
#endif

static void smp_Entry(IPTR stackBase, spinlock_t *apicReadyLock, struct KernelBase *KernelBase, apicid_t apicCPUNo)
{
    /*
     * This is the entry point for secondary cores.
     * KernelBase is already set up by the primary CPU, so we can use it.
     */
    struct APICData *apicData  = KernelBase->kb_PlatformData->kb_APIC;
    __unused struct CPUData *apicCPU;
    D(
        IPTR _APICBase;
        apicid_t _APICID;
    )
#if defined(__AROSEXEC_SMP__)
    struct Task *apicBSTask;
#endif
#if (__WORDSIZE==64)

    /* Enable fxsave/fxrstor */
    wrcr(cr4, rdcr(cr4) | _CR4_OSFXSR | _CR4_OSXMMEXCPT);
#endif

    apicCPU = &apicData->cores[apicCPUNo];

    D(
        /* Find out ourselves */
        _APICBase = core_APIC_GetBase();
        _APICID   = core_APIC_GetID(_APICBase);

        bug("[Kernel:SMP] %s[%03u]: APIC ID %03u starting up...\n", __func__, apicCPUNo, _APICID);
        if (apicCPU->cpu_LocalID != _APICID)
        {
            bug("[Kernel:SMP] %s[%03u]:        Warning! expected ID %03u\n", __func__, apicCPUNo, apicCPU->cpu_LocalID);
        }
        bug("[Kernel:SMP] %s[%03u]:     APIC base @ 0x%p\n", __func__, apicCPUNo, _APICBase);
#if (__WORDSIZE==64)
        bug("[Kernel:SMP] %s[%03u]:     KernelBootPrivate 0x%p\n", __func__, apicCPUNo, __KernBootPrivate);
#endif
        bug("[Kernel:SMP] %s[%03u]:     StackBase 0x%p\n", __func__, apicCPUNo, stackBase);
        bug("[Kernel:SMP] %s[%03u]:     Ready Lock 0x%p\n", __func__, apicCPUNo, apicReadyLock);
    )

#if (0)
    apicCPUNo   = core_APIC_GetNumber(apicData);
#endif

    D(bug("[Kernel:SMP] %s[%03u]: APIC CPU Data @ 0x%p\n", __func__, apicCPUNo, apicCPU));

    /* Set up GDT and LDT for our core */
    D(bug("[Kernel:SMP] %s[%03u]: GDT @ 0x%p, TLS @ 0x%p\n", __func__, apicCPUNo, apicCPU->cpu_GDT, apicCPU->cpu_TLS));
#if (__WORDSIZE==64)
    core_SetupGDT(__KernBootPrivate, apicCPUNo, apicCPU->cpu_GDT, apicCPU->cpu_TLS, __KernBootPrivate->TSS);

    core_CPUSetup(apicCPUNo, apicCPU->cpu_GDT, stackBase);
#endif

    D(bug("[Kernel:SMP] %s[%03u]: Core IDT @ 0x%p\n", __func__, apicCPUNo, apicCPU->cpu_IDT));
#if (__WORDSIZE==64)
    core_SetupIDT(apicCPUNo, apicCPU->cpu_IDT);

    if (!core_SetIDTGate((struct int_gate_64bit *)apicCPU->cpu_IDT, APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SYSCALL), (uintptr_t)IntrDefaultGates[APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SYSCALL)], TRUE, TRUE))
    {
        krnPanic(NULL, "Failed to set APIC Syscall Vector\n"
                       "Vector #$%02X\n",
                 APIC_CPU_EXCEPT_TO_VECTOR(APIC_EXCEPT_SYSCALL));
    }
    D(bug("[Kernel:SMP] %s[%03u]: APIC Syscall Vector configured\n", __func__, apicCPUNo));

    D(bug("[Kernel:SMP] %s[%03u]: Preparing MMU...\n", __func__, apicCPUNo));
    core_LoadMMU(&__KernBootPrivate->MMU);
#endif

#if defined(__AROSEXEC_SMP__)
    D(bug("[Kernel:SMP] %s[%03u]: SysBase @ 0x%p\n", __func__, apicCPUNo, SysBase));

    TLS_SET(SysBase,SysBase);
    TLS_SET(KernelBase,KernelBase);

    if ((apicBSTask = cpu_InitBootStrap(apicCPUNo)) != NULL)
    {
        apicBSTask->tc_SPLower = NULL;
        apicBSTask->tc_SPUpper = (APTR)~0;

        cpu_BootStrap(apicBSTask);
    }
#else
    bug("[Kernel:SMP] APIC #%u of %u Going IDLE (Halting)...\n", apicCPUNo + 1, apicData->apic_count);
#endif

    /* Signal the bootstrap core that we are running */
    KrnSpinUnLock((spinlock_t *)apicReadyLock);

#if defined(__AROSEXEC_SMP__)
    if (apicBSTask)
    {
        D(bug("[Kernel:SMP] %s[%03u]: Starting up Scheduler...\n", __func__, apicCPUNo);)

        /* clean up now we are done */
        RemTask(apicBSTask);
    }

    bug("[Kernel:SMP] APIC #%u Failed to bootstrap (Halting)...\n", apicCPUNo + 1);
    while (1) asm volatile("cli; hlt");
#else
    while (1) asm volatile("hlt");
#endif
}

static int smp_Setup(struct KernelBase *KernelBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    unsigned long bslen = (unsigned long)&_binary_smpbootstrap_size;
    struct MemHeader *lowmem;
    APTR smpboot = NULL;
    struct SMPBootstrap *bs;

    D(bug("[Kernel:SMP] %s()\n", __func__));

    /* Find a suitable memheader to allocate the bootstrap from .. */
    ForeachNode(&SysBase->MemList, lowmem)
    {
        /* Is it in lowmem? */
        if ((IPTR)lowmem->mh_Lower < 0x000100000)
        {
            D(bug("[Kernel:SMP] Trying memheader @ 0x%p\n", lowmem));
            D(bug("[Kernel:SMP] * 0x%p - 0x%p (%s pri %d)\n", lowmem->mh_Lower, lowmem->mh_Upper, lowmem->mh_Node.ln_Name, lowmem->mh_Node.ln_Pri));

            /*
             * Attempt to allocate space for the SMP bootstrap code.
             * NB:  Its address must be page-aligned!.
             * NB2: Every CPU starts up in real mode
             */
            smpboot = Allocate(lowmem, bslen + PAGE_SIZE - 1);
            if (smpboot)
                break;
        }
    }

    if (!smpboot)
    {
        bug("[Kernel:SMP] Failed to allocate %dbytes for SMP bootstrap\n", bslen + PAGE_SIZE - 1);
        return 0;
    }

    /* Install SMP bootstrap code */
    bs = (APTR)AROS_ROUNDUP2((IPTR)smpboot, PAGE_SIZE);
    CopyMem(&_binary_smpbootstrap_start, bs, (unsigned long)&_binary_smpbootstrap_size);
    pdata->kb_APIC_TrampolineBase = bs;

    D(bug("[Kernel:SMP] Copied APIC bootstrap code to 0x%p\n", bs));

    /*
     * Store constant arguments in bootstrap's data area
     * WARNING!!! The bootstrap code assumes PML4 is placed in a 32-bit memory,
     * and there seem to be no easy way to fix this.
     * If AROS kickstart is ever loaded into high memory, we would need to take
     * a special care about it.
     */
    bs->Arg3 = (IPTR)KernelBase;
#if (__WORDSIZE==64)
    //TODO: Allocate the cores own MMU structures and copy necessary data to it
    bs->PML4 = __KernBootPrivate->MMU.mmu_PML4;
#endif
    bs->IP   = smp_Entry;

    return 1;
}

/*
 * Here we wake up our secondary cores.
 */
static int smp_Wake(struct KernelBase *KernelBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct SMPBootstrap *bs = pdata->kb_APIC_TrampolineBase;
    struct APICData *apicData = pdata->kb_APIC;
    APTR _APICStackBase;
    IPTR wakeresult = -1;
    apicid_t cpuNo;
#if defined(__AROSEXEC_SMP__)
    tls_t *apicTLS;
#endif
    spinlock_t *apicReadyLocks;

    apicReadyLocks = AllocMem(sizeof(spinlock_t) * apicData->apic_count, MEMF_CLEAR|MEMF_ANY);
    D(bug("[Kernel:SMP] %d Ready spinlocks starting at 0x%p\n", apicData->apic_count, apicReadyLocks));

    /* Core number 0 is our bootstrap core, so we start from No 1 */
    for (cpuNo = 1; cpuNo < apicData->apic_count; cpuNo++)
    {
        struct APICCPUWake_Data apicWake = 
        {
            bs,
            apicData->lapicBase,
            apicData->cores[cpuNo].cpu_LocalID
        };

        DWAKE(bug("[Kernel:SMP] Launching CPU #%u (ID %03u)\n", cpuNo + 1, apicData->cores[cpuNo].cpu_LocalID));

        KrnSpinInit(&apicReadyLocks[cpuNo]);
        
        apicData->cores[cpuNo].cpu_GDT = PlatformAllocGDT(KernelBase, apicData->cores[cpuNo].cpu_LocalID);
        apicData->cores[cpuNo].cpu_TLS = PlatformAllocTLS(KernelBase, apicData->cores[cpuNo].cpu_LocalID);
#if defined(__AROSEXEC_SMP__)
        apicTLS = apicData->cores[cpuNo].cpu_TLS;
        apicTLS->ScheduleData = AllocMem(sizeof(struct X86SchedulerPrivate), MEMF_PUBLIC);
        core_InitScheduleData(apicTLS->ScheduleData);
        D(bug("[Kernel:SMP] Scheduling Data @ 0x%p\n", apicTLS->ScheduleData));
#endif
        apicData->cores[cpuNo].cpu_IDT = PlatformAllocIDT(KernelBase, apicData->cores[cpuNo].cpu_LocalID);

        /*
         * First we need to allocate a stack for our CPU.
         * We allocate the same three stacks as in core_CPUSetup().
         */
        _APICStackBase = AllocMem(STACK_SIZE * 3, MEMF_CLEAR);
        D(bug("[Kernel:SMP] Allocated STACK for APIC ID %03u @ 0x%p ..\n", apicData->cores[cpuNo].cpu_LocalID, _APICStackBase));
        if (!_APICStackBase)
                return 0;

        /* Pass some vital information to the
         * waking CPU */
        bs->Arg1 = (IPTR)_APICStackBase;
        bs->Arg2 = (IPTR)&apicReadyLocks[cpuNo];
        // Arg3 = KernelBase - already set by smp_Setup()
        bs->Arg4 = (IPTR)cpuNo;
        bs->SP   = _APICStackBase + STACK_SIZE;

        /* Lock the spinlock before launching the core */
        KrnSpinLock(&apicReadyLocks[cpuNo], NULL, SPINLOCK_MODE_WRITE);

        /* Start IPI sequence */
        wakeresult = krnSysCallCPUWake(&apicWake);

        /* wakeresult != 0 means error */
        if (!wakeresult)
        {
            UQUAD current, start = RDTSC();
            /*
             * Before we proceed we need to make sure that the core has picked up
             * its stack and we can reload bootstrap argument area with another one.
             */
            DWAKE(bug("[Kernel:SMP] Waiting for CPU #%u to initialise .. ", cpuNo + 1));
            while (!KrnSpinTryLock(&apicReadyLocks[cpuNo], SPINLOCK_MODE_READ))
            {
                asm volatile("pause");
                current = RDTSC();
                if (((current - start)/apicData->cores[0].cpu_TSCFreq) >
#if (DEBUG > 0)
                    50000
#else
                    50
#endif
                )
                {
                    wakeresult = -1;
                    break;
                }
            };
            if (wakeresult != -1)
            {
                KrnSpinUnLock(&apicReadyLocks[cpuNo]);
                DWAKE(bug("[Kernel:SMP] CPU #%u started up\n", cpuNo + 1));
            }
        }
        D(if (wakeresult) { bug("[Kernel:SMP] core_APIC_Wake() failed, status 0x%p\n", wakeresult); } ) 
    }

    D(bug("[Kernel:SMP] Done\n"));

    return 1;
}

int smp_Initialize(void)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    if (pdata->kb_APIC && (pdata->kb_APIC->apic_count > 1))
    {
        int number_of_ipi_messages = 0;
        struct IPIHook *hooks;
        int i;

#if defined(__AROSEXEC_SMP__)
        cpu_PrepareExec(SysBase);
#endif

        D(bug("[Kernel:SMP] %s: Initializing Lists for IPI messages ...\n", __func__));
        NEWLIST(&pdata->kb_FreeIPIHooks);
        NEWLIST(&pdata->kb_BusyIPIHooks);
        KrnSpinInit(&pdata->kb_FreeIPIHooksLock);
        KrnSpinInit(&pdata->kb_BusyIPIHooksLock);
        
        number_of_ipi_messages = pdata->kb_APIC->apic_count * 10;
        D(bug("[Kernel:SMP] %s: Allocating %d IPI CALL_HOOK messages ...\n", __func__, number_of_ipi_messages));
        hooks = AllocMem((sizeof(struct IPIHook) * number_of_ipi_messages + 127), MEMF_PUBLIC | MEMF_CLEAR);
        hooks = (struct IPIHook *)(((IPTR)hooks + 127) & ~127);
        if (hooks)
        {
            for (i=0; i < number_of_ipi_messages; i++)
            {
                hooks[i].ih_CPUDone = KrnAllocCPUMask();
                hooks[i].ih_CPURequested = KrnAllocCPUMask();
                KrnSpinInit(&hooks[i].ih_Lock);

                ADDHEAD(&pdata->kb_FreeIPIHooks, &hooks[i]);
            }
        }
        else
        {
            bug("[Kernel:SMP] %s: Failed to get IPI slots!\n", __func__);
        }

        if (!smp_Setup(KernelBase))
        {
            D(bug("[Kernel:SMP] Failed to prepare the environment!\n"));

            pdata->kb_APIC->apic_count = 1;	/* We have only one working CPU */
            return 0;
        }

        return smp_Wake(KernelBase);
    }

    /* This is not an SMP machine, but it's okay */
    return 1;
}
