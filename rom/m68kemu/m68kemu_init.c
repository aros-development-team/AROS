/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder
*/
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder

    m68kemu.library — library init/open/close
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "m68kemu_intern.h"

static int M68KEmu_Init(struct M68KEmuLibBase *base)
{
    base->sysBase = (struct ExecBase *)SysBase;
    base->dosBase = NULL;
    return TRUE;
}

static int M68KEmu_Open(struct M68KEmuLibBase *base)
{
    if (!base->dosBase)
        base->dosBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    return base->dosBase != NULL;
}

static void M68KEmu_Close(struct M68KEmuLibBase *base)
{
    /* Keep dos.library open for reuse */
}

static int M68KEmu_Expunge(struct M68KEmuLibBase *base)
{
    if (base->dosBase)
    {
        CloseLibrary((struct Library *)base->dosBase);
        base->dosBase = NULL;
    }
    return TRUE;
}

ADD2INITLIB(M68KEmu_Init, 0)
ADD2OPENLIB(M68KEmu_Open, 0)
ADD2CLOSELIB(M68KEmu_Close, 0)
ADD2EXPUNGELIB(M68KEmu_Expunge, 0)
