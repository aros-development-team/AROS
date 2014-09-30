/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
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
#include "apic.h"
#include "xtpic.h"

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

    return TRUE;
}

ADD2INITLIB(Platform_Init, 10)
