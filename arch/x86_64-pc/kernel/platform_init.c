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

#define D(x) x
#define DAPIC(x)

/* Post exec init */

static int Platform_Init(struct KernelBase *LIBBASE)
{
    struct PlatformData *pdata;
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
