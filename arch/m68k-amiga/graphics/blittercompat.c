/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/symbolsets.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <defines/graphics_LVO.h>

extern void AmigaOwnBlitter(void);
extern void AmigaDisownBlitter(void);

static int AmigaBlitterABI_Init(struct GfxBase *GfxBase)
{
    struct ExecBase *SysBase = GfxBase->ExecBase;

    SetFunction((struct Library *)GfxBase, -6 * LVOOwnBlitter,
                (APTR)AmigaOwnBlitter);
    SetFunction((struct Library *)GfxBase, -6 * LVODisownBlitter,
                (APTR)AmigaDisownBlitter);
    return TRUE;
}

ADD2INITLIB(AmigaBlitterABI_Init, 0);
