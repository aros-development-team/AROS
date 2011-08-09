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

/* Post exec init */

static int Platform_Init(struct KernelBase *LIBBASE)
{
    struct PlatformData *pd;
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

    pd->kb_APIC_Count   = 1;		/* We already have one running processor */
    pd->kb_APIC_IDMap   = AllocMem(sizeof(UWORD), MEMF_ANY);
    pd->kb_APIC_BaseMap = AllocMem(sizeof(IPTR), MEMF_ANY);

    D(bug("[Kernel] Kernel_Init: APIC IDMap @ %p, BaseMap @ %p\n", pd->kb_APIC_IDMap, pd->kb_APIC_BaseMap));

    pd->kb_APIC_IDMap[0]   = __KernBootPrivate->kbp_APIC_BSPID;
    pd->kb_APIC_BaseMap[0] = __KernBootPrivate->_APICBase;

    D(bug("[Kernel] Kernel_Init: BSP APIC ID %d, Base @ %p\n", pd->kb_APIC_IDMap[0], pd->kb_APIC_BaseMap[0]));

    core_APIC_Init(__KernBootPrivate->_APICBase);
    D(bug("[Kernel] APIC initialized\n"));

    return TRUE;
}

ADD2INITLIB(Platform_Init, 10)
