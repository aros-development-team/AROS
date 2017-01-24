/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

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
#include "smp.h"
#include "xtpic.h"
#include "tls.h"

#define D(x) x
#define DAPIC(x)

/* Post exec init */

static int Platform_Init(struct KernelBase *LIBBASE)
{
    struct PlatformData *pdata;
    int i;

    D(bug("[Kernel:x86_64] Kernel_Init: Post-exec init. KernelBase @ %p\n", LIBBASE));

    for (i = 0; i < HW_IRQ_COUNT; i++)
    {
        switch(i)
        {
            default:
                LIBBASE->kb_Interrupts[i].lh_Type = KBL_INTERNAL;
                break;
        }
    }

    D(bug("[Kernel:x86_64] Kernel_Init: Interupt List initialised\n"));

    pdata = AllocMem(sizeof(struct PlatformData), MEMF_PUBLIC|MEMF_CLEAR);
    if (!pdata)
    	return FALSE;

    LIBBASE->kb_PlatformData = pdata;

    return TRUE;
}

ADD2INITLIB(Platform_Init, 10)

void PlatformPostInit(void)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    ACPICABase = OpenLibrary("acpica.library", 0);

    if (ACPICABase)
        acpi_Init(pdata);

    // Now initialize our interrupt controller (XT-PIC or APIC)
    ictl_Initialize();

    // The last thing to do is to start up secondary CPU cores (if any)
    smp_Initialize();
}

APTR PlatformAllocGDT(struct KernelBase *LIBBASE, apicid_t _APICID)
{
    APTR GDTalloc;
    
    GDTalloc = (APTR)AllocMem(sizeof(struct gdt_64bit) + 128, MEMF_24BITDMA|MEMF_CLEAR);
    GDTalloc = (APTR)AROS_ROUNDUP2((unsigned long)GDTalloc, 128);
    D(bug("[Kernel] %s[%d]: GDT @ 0x%p\n", __func__, _APICID, GDTalloc));

    return GDTalloc;
}

APTR PlatformAllocTLS(struct KernelBase *LIBBASE, apicid_t _APICID)
{
    APTR TLSalloc;

    TLSalloc = (APTR)AllocMem(sizeof(tls_t), MEMF_24BITDMA|MEMF_CLEAR);
    TLSalloc = (APTR)AROS_ROUNDUP2((unsigned long)TLSalloc, sizeof(APTR));
    D(bug("[Kernel] %s[%d]: TLS @ 0x%p\n", __func__, _APICID, TLSalloc));

    return TLSalloc;
}

APTR PlatformAllocIDT(struct KernelBase *LIBBASE, apicid_t _APICID)
{
    APTR IDTalloc;

    if (!(IDTalloc = IDT_GET()))
    {
        IDTalloc = (APTR)AllocMem(sizeof(struct int_gate_64bit) * 256, MEMF_24BITDMA|MEMF_CLEAR);
        IDTalloc = (APTR)AROS_ROUNDUP2((unsigned long)IDTalloc, 256);
    	IDT_SET(IDTalloc)

    	D(bug("[Kernel] %s[%d]: Allocated IDT at 0x%p\n", __func__, _APICID, IDTalloc));
    }
    return IDTalloc;
}
