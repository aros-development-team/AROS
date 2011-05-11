#define __KERNEL_NOLIBBASE__

#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "acpi.h"
#include "apic.h"

#define D(x) x
#define DAPIC(x)

#define CONFIG_LAPICS

extern const void *_binary_smpbootstrap_start;
extern const void *_binary_smpbootstrap_size;

/* Post exec init */

static int Platform_Init(struct KernelBase *LIBBASE)
{
    struct PlatformData *pd;
    APTR smpboot;
    int i;

    D(bug("[Kernel] Kernel_Init: Post-exec init. KernelBase @ %p\n", LIBBASE));

    for (i = 0; i < IRQ_COUNT; i++)
    {
        switch(i)
        {
            case 0x00 ... 0x0f:
                LIBBASE->kb_Interrupts[i].lh_Type = KBL_XTPIC;
                break;
            case 0xde:
                LIBBASE->kb_Interrupts[i].lh_Type = KBL_APIC;
                break;
            default:
                LIBBASE->kb_Interrupts[i].lh_Type = KBL_INTERNAL;
                break;
        }
    }

    D(bug("[Kernel] Kernel_Init: Interupt List initialised\n"));

    pd = AllocMem(sizeof(struct PlatformData), MEMF_PUBLIC|MEMF_CLEAR);
    if (!pd)
    	return FALSE;

    LIBBASE->kb_PlatformData = pd;

    pd->kb_XTPIC_Mask = 0xfffb;
    pd->kb_APIC_Count = 1;
    pd->kb_APIC_Ready = 1;

    pd->kb_APIC_DriverID = __KernBootPrivate->kbp_APIC_DriverID;
    pd->kb_APIC_Drivers  = __KernBootPrivate->kbp_APIC_Drivers;

    pd->kb_APIC_IDMap   = AllocVec(sizeof(UWORD), MEMF_CLEAR);
    pd->kb_APIC_BaseMap = AllocVec(sizeof(IPTR), MEMF_CLEAR);

    D(bug("[Kernel] Kernel_Init: APIC IDMap @ %p, BaseMap @ %p\n", pd->kb_APIC_IDMap, pd->kb_APIC_BaseMap));
    D(bug("[Kernel] Kernel_Init: APIC Drivers @ %p, Using No %d\n", pd->kb_APIC_Drivers, pd->kb_APIC_DriverID));

    pd->kb_APIC_IDMap[0]   = __KernBootPrivate->kbp_APIC_BSPID;
    pd->kb_APIC_BaseMap[0] = core_APIC_GetBase(pd);

    D(bug("[Kernel] Kernel_Init: BSP APIC ID %d, Base @ %p\n", pd->kb_APIC_IDMap[0], pd->kb_APIC_BaseMap[0]));

#ifdef CONFIG_LAPICS
    /*
     * SMP bootstrap must be copied to low memory, every CPU starts up in real mode (DAMN CRAP!!!)
     * Its address must be page-aligned
     */
    smpboot = Allocate(__KernBootPrivate->kbp_LowMem, (unsigned long)&_binary_smpbootstrap_size + PAGE_SIZE - 1);
    if (smpboot)
    {
        pd->kb_APIC_TrampolineBase = (APTR)AROS_ROUNDUP2((IPTR)smpboot, PAGE_SIZE);

    	CopyMem(&_binary_smpbootstrap_start, pd->kb_APIC_TrampolineBase, (unsigned long)&_binary_smpbootstrap_size);

    	/* HACK! Store the PML4 address in smp trampoline area */
        *(ULONG *)(pd->kb_APIC_TrampolineBase + 0x0014) = (unsigned long)PML4;
    }
    D(bug("[Kernel] Kernel_Init: Copied APIC bootstrap code to %p\n", pd->kb_APIC_TrampolineBase));
#endif

    core_APIC_Init(pd);
    D(bug("[Kernel] APIC initialized\n"));

    /*
     * TODO: perhaps kernel.resource is not a correct place for this.
     * I suggest to move this to acpi.resource.
     *		Pavel Fedin <pavel_fedin@mail.ru>
     */
    pd->kb_ACPIRSDP = __KernBootPrivate->kbp_ACPIRSDP;
    if (pd->kb_ACPIRSDP)
        core_ACPIInitialise(pd);

    /*
     * The last thing to do - wait for secondary cores to start up
     * TODO: perhaps this should be handled by processor.resource.
     */
    D(bug("[Kernel] %u APICs detected\n", pd->kb_APIC_Count));

    if (pd->kb_APIC_Count > 1)
    {
    	/* This variable will be incremented by threads running on secondary CPUs */
    	volatile UBYTE *apicready = &pd->kb_APIC_Ready;

	D(bug("[Kernel] Waiting for %d APICs to initialise ..\n", pd->kb_APIC_Count - 1));
	while (*apicready < pd->kb_APIC_Count)
	{
	    DAPIC(bug("[Kernel] %d of %d APICs Ready ..\n", *apicready, pd->kb_APIC_Count));
	}
    }

    D(bug("[Kernel] Kernel_Init: Done\n"));

    return TRUE;
}

ADD2INITLIB(Platform_Init, 10)
