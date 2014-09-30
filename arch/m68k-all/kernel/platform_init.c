/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__

#include <aros/multiboot.h>
#include <aros/symbolsets.h>
#include <asm/cpu.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"

static int Platform_Init(struct KernelBase *LIBBASE)
{
    struct PlatformData *pd;

    pd = AllocMem(sizeof(struct PlatformData), MEMF_PUBLIC|MEMF_CLEAR);
    if (!pd)
    	return FALSE;

    LIBBASE->kb_PlatformData = pd;

    if (SysBase->AttnFlags & AFF_68060)
    	pd->mmu_type = MMU060;
    else if (SysBase->AttnFlags & AFF_68040)
    	pd->mmu_type = MMU040;
    else if (SysBase->AttnFlags & AFF_68030)
    	pd->mmu_type = MMU030;

    return TRUE;
}

ADD2INITLIB(Platform_Init, 10)
