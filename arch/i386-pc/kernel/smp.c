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
#include "kernel_intern.h"
#include "kernel_syscall.h"
#include "apic.h"
#include "smpbootstrap.h"

#define D(x)
#define DWAKE(x)

extern const void *_binary_smpbootstrap_start;
extern const void *_binary_smpbootstrap_size;

static void smp_Entry(volatile UBYTE *apicready)
{
    /*
     * This is the entry point for secondary cores.
     * KernelBase is already set up by the primary CPU, so we can use it.
     */
    D(
        IPTR _APICBase;
        UWORD _APICID;
    )
    UBYTE _APICNO;

    /* Find out ourselves */
    _APICNO   = core_APIC_GetNumber(KernelBase->kb_PlatformData->kb_APIC);

    D(
        _APICBase = core_APIC_GetBase();
        _APICID   = core_APIC_GetID(_APICBase);

        bug("[SMP] smp_Entry[0x%02X]: APIC base @ 0x%p\n", _APICID, _APICBase);
        bug("[SMP] smp_Entry[0x%02X]: Ready lock 0x%p\n", _APICID, apicready);
    )
    bug("[SMP] APIC #%u of %u Going IDLE (Halting)...\n", _APICNO + 1, KernelBase->kb_PlatformData->kb_APIC->apic_count);

    /* Signal the bootstrap core that we are running */
    *apicready = 1;

    /*
     * Unfortunately at the moment we have nothing more to do.
     * The rest of AROS is not SMP-capable. :-(
     */
    while (1) asm volatile("hlt");
}

int smp_Setup(void)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    /* Low memory header is in the tail of memory list - see kernel_startup.c */
    unsigned long bslen = (unsigned long)&_binary_smpbootstrap_size;
    struct MemHeader *lowmem;
    APTR smpboot = NULL;
    struct SMPBootstrap *bs;

    D(bug("[SMP] %s()\n", __func__));

    /* Find a suitable memheader to allocate the bootstrap from .. */
    ForeachNode(&SysBase->MemList, lowmem)
    {
        /* Is it in lowmem? */
        if ((IPTR)lowmem->mh_Lower < 0x000100000)
        {
            D(bug("[SMP] Trying memheader @ 0x%p\n", lowmem));
            D(bug("[SMP] * 0x%p - 0x%p (%s pri %d)\n", lowmem->mh_Lower, lowmem->mh_Upper, lowmem->mh_Node.ln_Name, lowmem->mh_Node.ln_Pri));

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
        bug("[SMP] Failed to allocate %dbytes for SMP bootstrap\n", bslen + PAGE_SIZE - 1);
        return 0;
    }

    /* Install SMP bootstrap code */
    bs = (APTR)AROS_ROUNDUP2((IPTR)smpboot, PAGE_SIZE);
    CopyMem(&_binary_smpbootstrap_start, bs, (unsigned long)&_binary_smpbootstrap_size);
    pdata->kb_APIC_TrampolineBase = bs;

    D(bug("[SMP] Copied APIC bootstrap code to 0x%p\n", bs));

    bs->IP = smp_Entry;
    return 1;
}

/*
 * Here we wake up our secondary cores.
 */
int smp_Wake(void)
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
    for (i = 1; i < apic->apic_count; i++)
    {
    	UBYTE apic_id = apic->cores[i].lapicID;

    	D(bug("[SMP] Launching APIC #%u (ID 0x%02X)\n", i + 1, apic_id));
 
	/* First we need to allocate a stack for our CPU. */
	_APICStackBase = AllocMem(STACK_SIZE, MEMF_CLEAR);
	D(bug("[SMP] Allocated STACK for APIC ID 0x%02X @ 0x%p\n", apic_id, _APICStackBase));
	if (!_APICStackBase)
		return 0;

	/* Give the stack to the CPU */
	bs->Arg1 = (IPTR)&apicready;
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
	    DWAKE(bug("[SMP] Waiting for APIC #%u to initialise .. ", i + 1));
	    while (!apicready);

	    D(bug("[SMP] APIC #%u started up\n", i + 1));
	}
	    D(else bug("[SMP] APIC wake() failed, status 0x%p\n", wakeresult));
    }

    D(bug("[SMP] Done\n"));

    return 1;
}
