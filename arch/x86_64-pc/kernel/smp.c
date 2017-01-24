/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/atomic.h>
#include <asm/io.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_intern.h"
#include "kernel_syscall.h"
#include "apic.h"
#include "smp.h"

#define D(x)
#define DWAKE(x)

extern const void *_binary_smpbootstrap_start;
extern const void *_binary_smpbootstrap_size;

extern APTR PlatformAllocGDT(struct KernelBase *, apicid_t);
extern APTR PlatformAllocTLS(struct KernelBase *, apicid_t);
extern APTR PlatformAllocIDT(struct KernelBase *, apicid_t);

static void smp_Entry(IPTR stackBase, volatile UBYTE *apicready, struct KernelBase *KernelBase)
{
    /*
     * This is the entry point for secondary cores.
     * KernelBase is already set up by the primary CPU, so we can use it.
     */
    APTR CORETLS, COREGDT, COREIDT;
    IPTR _APICBase;
    apicid_t _APICID;
    apicid_t _APICNO;

    /* Enable fxsave/fxrstor */
    wrcr(cr4, rdcr(cr4) | _CR4_OSFXSR | _CR4_OSXMMEXCPT);

    /* Find out ourselves */
    _APICBase = core_APIC_GetBase();
    _APICID   = core_APIC_GetID(_APICBase);
    _APICNO   = core_APIC_GetNumber(KernelBase->kb_PlatformData->kb_APIC);

    D(bug("[Kernel:SMP] smp_Entry[0x%02X]: launching on AP APIC ID 0x%02X, base @ 0x%p\n", _APICID, _APICID, _APICBase));
    D(bug("[Kernel:SMP] smp_Entry[0x%02X]: KernelBootPrivate 0x%p, stack base 0x%p\n", _APICID, __KernBootPrivate, stackBase));
    D(bug("[Kernel:SMP] smp_Entry[0x%02X]: Stack base 0x%p, ready lock 0x%p\n", _APICID, stackBase, apicready));

    /* Set up GDT and LDT for our core */
    CORETLS = PlatformAllocTLS(KernelBase, _APICID);
    COREGDT = PlatformAllocGDT(KernelBase, _APICID);
    COREIDT = PlatformAllocIDT(KernelBase, _APICID);

    D(bug("[Kernel:SMP] smp_Entry[0x%02X]: Core IDT @ 0x%p, GDT @ 0x%p, TLS @ 0x%p\n", _APICID, COREIDT, COREGDT, CORETLS));

    core_SetupGDT(__KernBootPrivate, _APICID, COREGDT, CORETLS, __KernBootPrivate->TSS);

    core_CPUSetup(_APICID, COREGDT, stackBase);
    core_SetupIDT(__KernBootPrivate, _APICID, COREIDT);

    bug("[Kernel:SMP] APIC #%u of %u Going IDLE (Halting)...\n", _APICNO + 1, KernelBase->kb_PlatformData->kb_APIC->apic_count);

    /* Signal the bootstrap core that we are running */
    *apicready = 1;

    /*
     * Unfortunately at the moment we have nothing more to do.
     * The rest of AROS is not SMP-capable. :-(
     */
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
    bs->PML4 = __KernBootPrivate->PML4;
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
    struct APICData *apic = pdata->kb_APIC;
    APTR _APICStackBase;
    IPTR wakeresult;
    apicid_t i;
    volatile UBYTE apicready;

    D(bug("[Kernel:SMP] Ready spinlock at 0x%p\n", &apicready));

    /* Core number 0 is our bootstrap core, so we start from No 1 */
    for (i = 1; i < apic->apic_count; i++)
    {
    	apicid_t apic_id = apic->cores[i].lapicID;

    	D(bug("[Kernel:SMP] Launching APIC #%u (ID 0x%02X)\n", i + 1, apic_id));
 
	/*
	 * First we need to allocate a stack for our CPU.
	 * We allocate the same three stacks as in core_CPUSetup().
	 */
	_APICStackBase = AllocMem(STACK_SIZE * 3, MEMF_CLEAR);
	D(bug("[Kernel:SMP] Allocated STACK for APIC ID 0x%02X @ 0x%p ..\n", apic_id, _APICStackBase));
	if (!_APICStackBase)
		return 0;

	/* Give the stack to the CPU */
	bs->Arg1 = (IPTR)_APICStackBase;
	bs->Arg2 = (IPTR)&apicready;
	bs->SP   = _APICStackBase + STACK_SIZE;

	/* Initialize 'ready' flag to zero before launching the core */
	apicready = 0;

	/* Start IPI sequence */
	wakeresult = core_APIC_Wake(bs, apic_id, apic->lapicBase);
	/* wakeresult != 0 means error */
	if (!wakeresult)
	{
	    /*
	     * Before we proceed we need to make sure that the core has picked up
	     * its stack and we can reload bootstrap argument area with another one.
	     * We use a very simple spinlock in order to perform this waiting.
	     * Previously we have set apicready to 0. When the core starts up,
	     * it writes 1 there.
	     */
	    DWAKE(bug("[Kernel:SMP] Waiting for APIC #%u to initialise .. ", i + 1));
	    while (!apicready);

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

    	    pdata->kb_APIC->apic_count = 1;	/* We have only one workinng CPU */
    	    return 0;
    	}

    	return smp_Wake(KernelBase);
    }

    /* This is not an SMP machine, but it's okay */
    return 1;
}
