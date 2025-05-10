/*
    Copyright (C) 2017-2023, The AROS Development Team. All rights reserved.
*/

#define __KERNEL_NOLIBBASE__

#include <proto/exec.h>
#include <proto/acpica.h>
#include <proto/kernel.h>

#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <hardware/pit.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <exec/resident.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_ipi.h"
#include "kernel_timer.h"
#include "acpi.h"
#include "apic.h"
#include "smp.h"

#include "x86_syscalls.h"

#define D(x)
//#define TEST_SMP_IPI

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
 * multitasking is enabled. It also means
 * acpica's "full initialization" task will have
 * been run.
 */

extern void kernelpost_end(void);

static AROS_UFP3 (APTR, KernelPost,
                  AROS_UFPA(struct Library *, lh, D0),
                  AROS_UFPA(BPTR, segList, A0),
                  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT kernelpost_namestring[] = "kernel.post";
static const TEXT kernelpost_versionstring[] = "kernel.post 1.2\n";

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

#if defined(TEST_SMP_IPI)
struct Hook test_ipi;

AROS_UFH3(void, test_ipi_hook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, object, A2),
    AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT

    int cpunum = KrnGetCPUNumber();

    bug("%s: called on CPU %d, hook=%p, object=%p, message=%p\n", __func__, cpunum, hook, object, message);

    AROS_USERFUNC_EXIT
}
#endif

static AROS_UFH3 (APTR, KernelPost,
                  AROS_UFHA(struct Library *, lh, D0),
                  AROS_UFHA(BPTR, segList, A0),
                  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct KernelBase *KernelBase;
    struct PlatformData *pdata;

    KernelBase = (struct KernelBase *)OpenResource("kernel.resource");
    if (!KernelBase)
            return NULL;

    pdata = KernelBase->kb_PlatformData;

    D(bug("[Kernel] %s: Checking for ACPI...\n", __func__));
    
    ACPICABase = OpenLibrary("acpica.library", 0);
    /* Probe for ACPI configuration */
    if (ACPICABase)
        acpi_Init(pdata);

    D(bug("[Kernel] %s: Performing late system configuration...\n", __func__));

    Disable();

    // Add the default reboot/shutdown handlers if ACPI ones haven't been registered
    krnAddSysCallHandler(pdata, &x86_SCSysHaltHandler, TRUE, FALSE);
    krnAddSysCallHandler(pdata, &x86_SCRebootHandler, TRUE, FALSE);
    krnAddSysCallHandler(pdata, &x86_SCChangePMStateHandler, TRUE, FALSE);

    D(bug("[Kernel] %s: Initializing interrupt controllers...\n", __func__));
    ictl_Initialize(KernelBase);

#if (__WORDSIZE!=64)
    /* Enable APIC on i386. x86_64 enables it much earlier, but APIC is guaranteed on x86_64 */
    if (pdata->kb_APIC && pdata->kb_APIC->lapicBase)
    {
        core_APIC_Config(pdata->kb_APIC->lapicBase, 0);
        core_APIC_Enable(pdata->kb_APIC->lapicBase, 0);
    }
#endif

    // Calibrate the BSP
    if (pdata->kb_APIC)
    {
        pit_start(0);
        core_APIC_Calibrate(pdata->kb_APIC, 0);        
    }

    D(bug("[Kernel] %s: Attempting to bring up additional cores...\n", __func__));
    smp_Initialize();

    Enable();

    D(bug("[Kernel] %s: Platform Initialization complete\n", __func__));
    pdata->kb_PDFlags |= PLATFORMF_PRIMED;

#if defined(TEST_SMP_IPI)
    bug("[Kernel] %s: --- TESTING IPI CALL HOOK ---\n", __func__);
    test_ipi.h_Entry = test_ipi_hook;
    test_ipi.h_Data = KernelBase;
    bug("[Kernel] %s: --- SYNCHRONOUS IPI ---\n", __func__);
    core_DoCallIPI(&test_ipi, (void*)TASKAFFINITY_ALL_BUT_SELF, 0, KernelBase);
    bug("[Kernel] %s: --- ASYNC IPI ---\n", __func__);
    core_DoCallIPI(&test_ipi, (void*)TASKAFFINITY_ALL_BUT_SELF, 1, KernelBase);
#endif

    AROS_USERFUNC_EXIT

    return NULL;
}

void kernelpost_end(void) { };
