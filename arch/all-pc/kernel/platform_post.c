/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
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

#define D(x)

/* 
    This file contains code that is run once Exec has been brought up - and is launched
    via the RomTag/Autoinit routines in Exec.
    
    Here we do the Platform configuration that requires a working "AROS" environment.
*/

void PlatformPostInit(void)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;

    ACPICABase = OpenLibrary("acpica.library", 0);

    // Probe for ACPI configuration ...
    if (ACPICABase)
        acpi_Init(pdata);

    D(bug("[Kernel] %s: Attempting to bring up aditional cores ...\n", __func__));
    smp_Initialize();

    D(bug("[Kernel] %s: Initializing Interrupt Controllers ...\n", __func__));
    ictl_Initialize(KernelBase);

    D(bug("[Kernel] %s: Platform Initialization complete\n", __func__));
}

APTR PlatformAllocGDT(struct KernelBase *LIBBASE, apicid_t _APICID)
{
    APTR GDTalloc;
    
    GDTalloc = (APTR)AllocMem(GDT_SIZE, MEMF_24BITDMA|MEMF_CLEAR);
    GDTalloc = (APTR)AROS_ROUNDUP2((unsigned long)GDTalloc, 128);
    D(bug("[Kernel] %s[%d]: GDT @ 0x%p\n", __func__, _APICID, GDTalloc));

    return GDTalloc;
}

APTR PlatformAllocTLS(struct KernelBase *LIBBASE, apicid_t _APICID)
{
    APTR TLSalloc = NULL;

#if (__WORDSIZE==64)
    TLSalloc = (APTR)AllocMem(sizeof(tls_t), MEMF_24BITDMA|MEMF_CLEAR);
    TLSalloc = (APTR)AROS_ROUNDUP2((unsigned long)TLSalloc, sizeof(APTR));
#endif
    D(bug("[Kernel] %s[%d]: TLS @ 0x%p\n", __func__, _APICID, TLSalloc));

    return TLSalloc;
}

APTR PlatformAllocIDT(struct KernelBase *LIBBASE, apicid_t _APICID)
{
    APTR IDTalloc;

    if (!(IDTalloc = IDT_GET()))
    {
        IDTalloc = (APTR)AllocMem(IDT_SIZE, MEMF_24BITDMA|MEMF_CLEAR);
        IDTalloc = (APTR)AROS_ROUNDUP2((unsigned long)IDTalloc, 256);
    	IDT_SET(IDTalloc)

    	D(bug("[Kernel] %s[%d]: Allocated IDT at 0x%p\n", __func__, _APICID, IDTalloc));
    }
    return IDTalloc;
}
