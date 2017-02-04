/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
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
#include "apic.h"
#include "smp.h"

#define D(x) x
#define DWAKE(x)

extern const void *_binary_smpbootstrap_start;
extern const void *_binary_smpbootstrap_size;

extern APTR PlatformAllocGDT(struct KernelBase *, apicid_t);
extern APTR PlatformAllocTLS(struct KernelBase *, apicid_t);
extern APTR PlatformAllocIDT(struct KernelBase *, apicid_t);

#if defined(__AROSEXEC_SMP__)
extern struct Task *cpu_InitBootStrap();
extern void cpu_BootStrap(struct Task *);
#endif

static void smp_Entry(IPTR stackBase, spinlock_t *apicReadyLock, struct KernelBase *KernelBase)
{
    /*
     * This is the entry point for secondary cores.
     * KernelBase is already set up by the primary CPU, so we can use it.
     */
    struct APICData *apicData  = KernelBase->kb_PlatformData->kb_APIC;
    __unused struct CPUData *apicCPU;
    IPTR _APICBase;
    apicid_t _APICID;
    apicid_t _APICNO;
#if defined(__AROSEXEC_SMP__)
    struct Task *apicBSTask;
#endif

#if (__WORDSIZE==64)
    /* Enable fxsave/fxrstor */
    wrcr(cr4, rdcr(cr4) | _CR4_OSFXSR | _CR4_OSXMMEXCPT);
#endif

    /* Find out ourselves */
    _APICBase = core_APIC_GetBase();
    _APICID   = core_APIC_GetID(_APICBase);
    _APICNO   = core_APIC_GetNumber(apicData);

    D(
        bug("[Kernel:SMP] %s[0x%02X]: CPU Core starting up...\n", __func__, _APICID);
        bug("[Kernel:SMP] %s[0x%02X]:     APIC base @ 0x%p\n", __func__, _APICID, _APICBase);
#if (__WORDSIZE==64)
        bug("[Kernel:SMP] %s[0x%02X]:     KernelBootPrivate 0x%p\n", __func__, _APICID, __KernBootPrivate);
#endif
        bug("[Kernel:SMP] %s[0x%02X]:     StackBase 0x%p\n", __func__, _APICID, stackBase);
        bug("[Kernel:SMP] %s[0x%02X]:     Ready Lock 0x%p\n", __func__, _APICID, apicReadyLock);
    )

    apicCPU = &apicData->cores[_APICNO];

    D(bug("[Kernel:SMP] %s[0x%02X]: APIC CPU Data @ 0x%p\n", __func__, _APICID, apicCPU));

    /* Set up GDT and LDT for our core */
    D(bug("[Kernel:SMP] %s[0x%02X]: GDT @ 0x%p, TLS @ 0x%p\n", __func__, _APICID, apicCPU->cpu_GDT, apicCPU->cpu_TLS));
#if (__WORDSIZE==64)
    core_SetupGDT(__KernBootPrivate, _APICNO, apicCPU->cpu_GDT, apicCPU->cpu_TLS, __KernBootPrivate->TSS);

    core_CPUSetup(_APICNO, apicCPU->cpu_GDT, stackBase);
#endif

    D(bug("[Kernel:SMP] %s[0x%02X]: Core IDT @ 0x%p\n", __func__, _APICID, apicCPU->cpu_IDT));
#if (__WORDSIZE==64)
    core_SetupIDT(__KernBootPrivate, _APICNO, apicCPU->cpu_IDT);

#if (0)
    D(bug("[Kernel:SMP] %s[0x%02X]: Preparing MMU...\n", __func__, _APICID));
    core_LoadMMU(&__KernBootPrivate->MMU);
#endif

    D(bug("[Kernel:SMP] %s[0x%02X]: Initialising APIC...\n", __func__, _APICID));
    core_APIC_Init(apicData, _APICNO);
#endif

#if defined(__AROSEXEC_SMP__)
    D(bug("[Kernel:SMP] %s[0x%02X]: SysBase @ 0x%p\n", __func__, _APICID, SysBase));

    TLS_SET(SysBase,SysBase);
    TLS_SET(KernelBase,KernelBase);

    if ((apicBSTask = cpu_InitBootStrap()) != NULL)
    {
        cpu_BootStrap(apicBSTask);
#else
    bug("[Kernel:SMP] APIC #%u of %u Going IDLE (Halting)...\n", _APICNO + 1, apicData->apic_count);
#endif

    /* Signal the bootstrap core that we are running */
    KrnSpinUnLock((spinlock_t *)apicReadyLock);

#if defined(__AROSEXEC_SMP__)
    }

    bug("[Kernel:SMP] APIC #%u Failed to bootstrap (Halting)...\n", _APICNO + 1);
#endif
    while (1) asm volatile("hlt");
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
    apicid_t i;
    spinlock_t apicReadyLock;

    D(bug("[Kernel:SMP] Ready spinlock at 0x%p\n", &apicReadyLock));

    KrnSpinInit(&apicReadyLock);

    /* Core number 0 is our bootstrap core, so we start from No 1 */
    for (i = 1; i < apicData->apic_count; i++)
    {
        struct APICCPUWake_Data apicWake = 
        {
            bs,
            apicData->lapicBase,
            apicData->cores[i].cpu_LocalID
        };

        D(bug("[Kernel:SMP] Launching APIC #%u (ID 0x%02X)\n", i + 1, apicData->cores[i].cpu_LocalID));
 
        apicData->cores[i].cpu_GDT = PlatformAllocGDT(KernelBase, apicData->cores[i].cpu_LocalID);
        apicData->cores[i].cpu_TLS = PlatformAllocTLS(KernelBase, apicData->cores[i].cpu_LocalID);
        apicData->cores[i].cpu_IDT = PlatformAllocIDT(KernelBase, apicData->cores[i].cpu_LocalID);

        /*
         * First we need to allocate a stack for our CPU.
         * We allocate the same three stacks as in core_CPUSetup().
         */
        _APICStackBase = AllocMem(STACK_SIZE * 3, MEMF_CLEAR);
        D(bug("[Kernel:SMP] Allocated STACK for APIC ID 0x%02X @ 0x%p ..\n", apicData->cores[i].cpu_LocalID, _APICStackBase));
        if (!_APICStackBase)
                return 0;

        /* Pass some vital information to the
         * waking CPU */
        bs->Arg1 = (IPTR)_APICStackBase;
        bs->Arg2 = (IPTR)&apicReadyLock;
        // Arg3 = KernelBase - already set by smp_Setup()
        bs->Arg4 = (IPTR)i;
        bs->SP   = _APICStackBase + STACK_SIZE;

        /* Lock the spinlock before launching the core */
        KrnSpinLock(&apicReadyLock, NULL, SPINLOCK_MODE_WRITE);

        /* Start IPI sequence */
        wakeresult = krnSysCallCPUWake(&apicWake);

        /* wakeresult != 0 means error */
        if (!wakeresult)
        {
            /*
             * Before we proceed we need to make sure that the core has picked up
             * its stack and we can reload bootstrap argument area with another one.
             */
            DWAKE(bug("[Kernel:SMP] Waiting for APIC #%u to initialise .. ", i + 1));
            while (!KrnSpinTryLock(&apicReadyLock, SPINLOCK_MODE_READ)){};
            KrnSpinUnLock(&apicReadyLock);
            D(bug("[Kernel:SMP] APIC #%u started up\n", i + 1));
        }
        D(else bug("[Kernel:SMP] core_APIC_Wake() failed, status 0x%p\n", wakeresult));
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
