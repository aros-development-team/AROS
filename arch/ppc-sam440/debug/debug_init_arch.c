/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/debug.h>
#include <libraries/debug.h>

#include "debug_intern.h"
#undef KernelBase

#define module_t parthenope_module_t
#include "kernel_intern.h"
#undef module_t

#include "kernel_base.h"

static int Debug_Init_Arch(struct Library *DebugBase)
{
    struct KernelBase *KernelBase = DBGBASE(DebugBase)->db_KernelBase;
    if (KernelBase && KernelBase->kb_PlatformData &&
        KernelBase->kb_PlatformData->pd_DebugInfo) {
        D(bug("[Debug] Adding KickStart module info at %p\n", KernelBase->kb_PlatformData->pd_DebugInfo));
        RegisterModule("KickStart", NULL, DEBUG_PARTHENOPE, KernelBase->kb_PlatformData->pd_DebugInfo);
    }

    return 1;
}

ADD2INITLIB(Debug_Init_Arch, 127)
