/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__

#include <proto/exec.h>
#include <proto/acpica.h>
#include <proto/kernel.h>

#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <exec/resident.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_ipi.h"
#include "acpi.h"
#include "apic.h"
#include "smp.h"

#define D(x) x

/* 
 * This file contains code that is run once Exec has been brought up - and is launched
 * via the RomTag/Autoinit routines in Exec.
 *
 * PlatformPostInit is run during RTF_SINGLETASK.
 */

void PlatformPostInit(void)
{
    D(bug("[Kernel] %s()\n", __func__));
}

/*
    Here we do the Platform configuration that requires a working "AROS" environment.
*/

APTR PlatformAllocGDT(struct KernelBase *LIBBASE, apicid_t _APICID)
{
    APTR GDTalloc;
    
    GDTalloc = (APTR)AllocMem(GDT_SIZE + 128, MEMF_24BITDMA|MEMF_CLEAR);
    GDTalloc = (APTR)AROS_ROUNDUP2((unsigned long)GDTalloc, 128);
    D(bug("[Kernel] %s[%d]: GDT @ 0x%p\n", __func__, _APICID, GDTalloc));

    return GDTalloc;
}

APTR PlatformAllocTLS(struct KernelBase *LIBBASE, apicid_t _APICID)
{
    APTR TLSalloc = NULL;

    TLSalloc = (APTR)AllocMem(TLS_SIZE + TLS_ALIGN, MEMF_24BITDMA|MEMF_CLEAR);
    TLSalloc = (APTR)AROS_ROUNDUP2((unsigned long)TLSalloc, TLS_ALIGN);

    D(bug("[Kernel] %s[%d]: TLS @ 0x%p\n", __func__, _APICID, TLSalloc));

    return TLSalloc;
}

APTR PlatformAllocIDT(struct KernelBase *LIBBASE, apicid_t _APICID)
{
    APTR IDTalloc;

    IDTalloc = (APTR)AllocMem(IDT_SIZE + 256, MEMF_24BITDMA|MEMF_CLEAR);
    IDTalloc = (APTR)AROS_ROUNDUP2((unsigned long)IDTalloc, 256);

    D(bug("[Kernel] %s[%d]: Allocated IDT at 0x%p\n", __func__, _APICID, IDTalloc));

    return IDTalloc;
}

/*
 * kernel.post is run during RTF_COLDSTART
 * directly after exec.library.
 *
 * At this point exec is fully configured, and
 * and multitasking is enabled. It also means
 * acpica's "full initialization" task will have
 * been run.
 */

extern void kernelpost_end(void);

static AROS_UFP3 (APTR, KernelPost,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT kernelpost_namestring[] = "kernel.post";
static const TEXT kernelpost_versionstring[] = "kernel.post 1.1\n";

const struct Resident kernelpost_romtag =
{
   RTC_MATCHWORD,
   (struct Resident *)&kernelpost_romtag,
   (APTR)&kernelpost_end,
   RTF_COLDSTART,
   1,
   NT_UNKNOWN,
   115,
   (STRPTR)kernelpost_namestring,
   (STRPTR)kernelpost_versionstring,
   (APTR)KernelPost
};

extern struct syscallx86_Handler x86_SCRebootHandler;
extern struct syscallx86_Handler x86_SCChangePMStateHandler;

static AROS_UFH3 (APTR, KernelPost,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    int number_of_ipi_messages = 0;
    struct KernelBase *KernelBase;
    struct PlatformData *pdata;
    struct IPIHook *hooks;
    int i;

    KernelBase = (struct KernelBase *)OpenResource("kernel.resource");
    if (!KernelBase)
            return NULL;

    pdata = KernelBase->kb_PlatformData;

    ACPICABase = OpenLibrary("acpica.library", 0);

    // Probe for ACPI configuration ...
    if (ACPICABase)
        acpi_Init(pdata);

    Disable();

    // Add the default reboot/shutdown handlers if ACPI ones havent been registered...
    krnAddSysCallHandler(pdata, &x86_SCRebootHandler, TRUE, FALSE);
    krnAddSysCallHandler(pdata, &x86_SCChangePMStateHandler, TRUE, FALSE);

    D(bug("[Kernel] %s: Initializing Lists for IPI messages ...\n", __func__));
    NEWLIST(&pdata->kb_FreeIPIHooks);
    NEWLIST(&pdata->kb_BusyIPIHooks);
    KrnSpinInit(&pdata->kb_FreeIPIHooksLock);
    KrnSpinInit(&pdata->kb_BusyIPIHooksLock);
    
    number_of_ipi_messages = pdata->kb_APIC->apic_count * 4;
    D(bug("[Kernel] %s: Allocating %d IPI CALL_HOOK messages ...\n", __func__, number_of_ipi_messages));
    hooks = AllocMem(sizeof(struct IPIHook) * number_of_ipi_messages, MEMF_PUBLIC | MEMF_CLEAR);
    if (hooks)
    {
        for (i=0; i < number_of_ipi_messages; i++)
        {
            hooks[i].ih_CPUDone = KrnAllocCPUMask();
            hooks[i].ih_CPURequested = KrnAllocCPUMask();

            ADDHEAD(&pdata->kb_FreeIPIHooks, &hooks[i]);
        }
    }
    else
    {
        bug("[Kernel] %s: Failed to get IPI slots!\n", __func__);
    }

    D(bug("[Kernel] %s: Attempting to bring up aditional cores ...\n", __func__));
    smp_Initialize();

    D(bug("[Kernel] %s: Initializing Interrupt Controllers ...\n", __func__));
    ictl_Initialize(KernelBase);

    Enable();

    D(bug("[Kernel] %s: Platform Initialization complete\n", __func__));

    AROS_USERFUNC_EXIT

    return NULL;
}

void kernelpost_end(void) { };
