#define DEBUG 1

#include <aros/debug.h>
#include <aros/multiboot.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/segments.h>
#include <inttypes.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <bootconsole.h>
#include <stdio.h>
#include <stdlib.h>

#include "kernel_intern.h"
#include LC_LIBDEFS_FILE

#define CONFIG_LAPICS

extern const void *_binary_smpbootstrap_start;
extern const void *_binary_smpbootstrap_size;

/* Post exec init */

static int Kernel_Init(LIBBASETYPEPTR LIBBASE)
{
    int i;

    struct ExecBase *SysBase = TLS_GET(SysBase);

    D(bug("[Kernel] Kernel_Init: Post-exec init. KernelBase @ %p\n", LIBBASE));
    TLS_SET(KernelBase, LIBBASE);

    LIBBASE->kb_XTPIC_Mask = 0xfffb;

    for (i=0; i < 256; i++)
    {
        NEWLIST(&LIBBASE->kb_Intr[i]);
        switch(i)
        {
            case 0x20 ... 0x2f:
                LIBBASE->kb_Intr[i].lh_Type = KBL_XTPIC;
                break;
            case 0xfe:
                LIBBASE->kb_Intr[i].lh_Type = KBL_APIC;
                break;
            default:
                LIBBASE->kb_Intr[i].lh_Type = KBL_INTERNAL;
                break;
        }
    }

    D(bug("[Kernel] Kernel_Init: Interupt List initialised\n"));

    LIBBASE->kb_APIC_Count = 1;

    LIBBASE->kb_APIC_DriverID = __KernBootPrivate->kbp_APIC_DriverID;
    LIBBASE->kb_APIC_Drivers = __KernBootPrivate->kbp_APIC_Drivers;

    LIBBASE->kb_APIC_IDMap = AllocVec(sizeof(UWORD), MEMF_CLEAR);
    LIBBASE->kb_APIC_BaseMap = AllocVec(sizeof(IPTR), MEMF_CLEAR);

    D(bug("[Kernel] Kernel_Init: APIC IDMap @ %p, BaseMap @ %p\n", LIBBASE->kb_APIC_IDMap, LIBBASE->kb_APIC_BaseMap));

    D(bug("[Kernel] Kernel_Init: APIC Drivers @ %p, Using No %d\n", LIBBASE->kb_APIC_Drivers, LIBBASE->kb_APIC_DriverID));

    LIBBASE->kb_APIC_IDMap[0] = __KernBootPrivate->kbp_APIC_BSPID;
    LIBBASE->kb_APIC_BaseMap[0] = AROS_UFC0(IPTR,
                ((struct GenericAPIC *)(LIBBASE->kb_APIC_Drivers[LIBBASE->kb_APIC_DriverID])->getbase));

    D(bug("[Kernel] Kernel_Init: BSP APIC ID %d, Base @ %p\n", LIBBASE->kb_APIC_IDMap[0], LIBBASE->kb_APIC_BaseMap[0]));

#ifdef CONFIG_LAPICS
    /* SMP bootstrap must be copied to low memory, every CPU starts up in real mode (DAMN CRAP!!!) */
    LIBBASE->kb_APIC_TrampolineBase = Allocate(__KernBootPrivate->kbp_LowMem, (unsigned long)&_binary_smpbootstrap_size);
    if (LIBBASE->kb_APIC_TrampolineBase)
    {
    	CopyMem(&_binary_smpbootstrap_start, LIBBASE->kb_APIC_TrampolineBase, (unsigned long)&_binary_smpbootstrap_size);

    	/* HACK! Store the PML4 address in smp trampoline area */
        *(ULONG *)(LIBBASE->kb_APIC_TrampolineBase + 0x0014) = (unsigned long)PML4;
    }
    D(bug("[Kernel] Kernel_Init: Copied APIC bootstrap code to %p\n", LIBBASE->kb_APIC_TrampolineBase));
#endif

     AROS_UFC1(IPTR, ((struct GenericAPIC *)LIBBASE->kb_APIC_Drivers[LIBBASE->kb_APIC_DriverID])->init,
            AROS_UFCA(IPTR, LIBBASE->kb_APIC_BaseMap[0], A0));

    D(bug("[Kernel] APIC initialized\n"));

    LIBBASE->kb_ACPIRSDP = __KernBootPrivate->kbp_ACPIRSDP;
    if (LIBBASE->kb_ACPIRSDP)
        core_ACPIInitialise(LIBBASE);

    D(bug("[Kernel] Kernel_Init: Done\n"));

    return TRUE;
}

ADD2INITLIB(Kernel_Init, 0)
