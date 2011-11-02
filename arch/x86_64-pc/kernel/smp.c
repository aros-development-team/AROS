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

#define D(x) x
#define DWAKE(x)

extern const void *_binary_smpbootstrap_start;
extern const void *_binary_smpbootstrap_size;

static void smp_Entry(IPTR stackBase, volatile UBYTE *apicready, struct KernelBase *KernelBase)
{
    /*
     * This is the entry point for secondary cores.
     * KernelBase is already set up by the primary CPU, so we can use it.
     */
    IPTR _APICBase;
    UWORD _APICID;
    UBYTE _APICNO;

    /* Enable fxsave/fxrstor */
    wrcr(cr4, rdcr(cr4) | _CR4_OSFXSR | _CR4_OSXMMEXCPT);

    /* Find out ourselves */
    _APICBase = core_APIC_GetBase();
    _APICID   = core_APIC_GetID(_APICBase);
    _APICNO   = core_APIC_GetNumber(KernelBase->kb_PlatformData->kb_APIC);

    D(bug("[SMP] smp_Entry[%d]: launching on AP APIC ID %d, base @ %p\n", _APICID, _APICID, _APICBase));
    D(bug("[SMP] smp_Entry[%d]: KernelBootPrivate 0x%p, stack base 0x%p\n", _APICID, __KernBootPrivate, stackBase));
    D(bug("[SMP] smp_Entry[%d]: Stack base 0x%p, ready lock 0x%p\n", _APICID, stackBase, apicready));

    /* Set up GDT and LDT for our core */
    core_CPUSetup(_APICID, stackBase);

    bug("[SMP] APIC No. %d of %d Going IDLE (Halting)...\n", _APICNO, KernelBase->kb_PlatformData->kb_APIC->count);

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
    /* Low memory header is in the tail of memory list - see kernel_startup.c */
    struct MemHeader *lowmem = (struct MemHeader *)SysBase->MemList.lh_TailPred;
    APTR smpboot;
    struct SMPBootstrap *bs;

    D(bug("[SMP] Setup\n"));

    /*
     * Allocate space for SMP bootstrap code in low memory. Its address must be page-aligned.
     * Every CPU starts up in real mode (DAMN CRAP!!!)
     */
    smpboot = Allocate(lowmem, (unsigned long)&_binary_smpbootstrap_size + PAGE_SIZE - 1);
    if (!smpboot)
    {
    	D(bug("[SMP] Failed to allocate space for SMP bootstrap\n"));
    	return 0;
    }

    /* Install SMP bootstrap code */
    bs = (APTR)AROS_ROUNDUP2((IPTR)smpboot, PAGE_SIZE);
    CopyMem(&_binary_smpbootstrap_start, bs, (unsigned long)&_binary_smpbootstrap_size);
    pdata->kb_APIC_TrampolineBase = bs;

    D(bug("[SMP] Copied APIC bootstrap code to %p\n", bs));

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
    UBYTE i;
    volatile UBYTE apicready;

    D(bug("[SMP] Ready spinlock at 0x%p\n", &apicready));

    /* Core number 0 is our bootstrap core, so we start from No 1 */
    for (i = 1; i < apic->count; i++)
    {
    	UBYTE apic_id = apic->cores[i].lapicID;

    	D(bug("[SMP] Launching APIC %u (ID %u)\n", i, apic_id));
 
	/*
	 * First we need to allocate a stack for our CPU.
	 * We allocate the same three stacks as in core_CPUSetup().
	 */
	_APICStackBase = AllocMem(STACK_SIZE * 3, MEMF_CLEAR);
	D(bug("[SMP] Allocated STACK for APIC ID %d @ %p ..\n", apic_id, _APICStackBase));
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
	    DWAKE(bug("[SMP] Waiting for APIC %u to initialise .. ", i));
	    while (!apicready);

	    D(bug("[SMP] APIC %u started up\n", i));
	}
	    D(else bug("[SMP] core_APIC_Wake() failed, status 0x%p\n", wakeresult));
    }

    D(bug("[SMP] Done\n"));

    return 1;
}

int smp_Initialize(void)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    if (pdata->kb_APIC && (pdata->kb_APIC->count > 1))
    {
    	if (!smp_Setup(KernelBase))
    	{
    	    D(bug("[SMP] Failed to prepare the environment!\n"));

    	    pdata->kb_APIC->count = 1;	/* We have only one workinng CPU */
    	    return 0;
    	}

    	return smp_Wake(KernelBase);
    }

    /* This is not an SMP machine, but it's okay */
    return 1;
}
