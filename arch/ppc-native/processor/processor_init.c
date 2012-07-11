/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/exec.h>

#include <asm/amcc440.h>

#include "processor_intern_arch.h"

static ULONG getpvr(void)
{
    return rdspr(PVR);
}

static int Processor_Init(struct ProcessorBase * ProcessorBase)
{
    struct SystemProcessors *sysprocs = AllocVec(sizeof(struct SystemProcessors), MEMF_ANY | MEMF_CLEAR);

    if (sysprocs == NULL)
        return FALSE;

    sysprocs->sp_PVR = Supervisor(getpvr);

    ProcessorBase->Private1 = sysprocs;

    return TRUE;
}

ADD2INITLIB(Processor_Init, 1);
