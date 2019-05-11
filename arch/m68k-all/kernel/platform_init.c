/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
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

#define D(x)

static int Platform_Init(struct KernelBase *KernelBase)
{
    struct PlatformData *pd;

    D(bug("[Kernel:m68k] %s()\n", __func__);)

    pd = AllocMem(sizeof(struct PlatformData), MEMF_PUBLIC|MEMF_CLEAR);
    if (!pd)
    	return FALSE;

    D(bug("[Kernel:m68k] %s: platformdata @ 0x%p\n", __func__, pd);)

    KernelBase->kb_PlatformData = pd;
    if (SysBase->AttnFlags & AFF_68080)
	pd->mmu_type = 0;
    else if (SysBase->AttnFlags & AFF_68060)
    	pd->mmu_type = MMU060;
    else if (SysBase->AttnFlags & AFF_68040)
    	pd->mmu_type = MMU040;
    else if (SysBase->AttnFlags & AFF_68030)
    	pd->mmu_type = MMU030;

    D(
      if (pd->mmu_type)
          bug("[Kernel:m68k] %s: mmu type set to #%d\n", __func__, pd->mmu_type);
     )

    return TRUE;
}

ADD2INITLIB(Platform_Init, 10)
