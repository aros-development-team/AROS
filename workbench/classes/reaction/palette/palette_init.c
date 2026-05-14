/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: palette.gadget class library init/expunge - opens bevel.image.
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "palette_intern.h"

static int Palette_InitLib(struct PaletteBase_intern *base)
{
    base->rc_BevelBase = OpenLibrary("images/bevel.image", 0);
    if (!base->rc_BevelBase)
        base->rc_BevelBase = OpenLibrary("bevel.image", 0);
    return TRUE;
}

static int Palette_ExpungeLib(struct PaletteBase_intern *base)
{
    if (base->rc_BevelBase)
    {
        CloseLibrary(base->rc_BevelBase);
        base->rc_BevelBase = NULL;
    }
    return TRUE;
}

ADD2INITLIB(Palette_InitLib, 0);
ADD2EXPUNGELIB(Palette_ExpungeLib, 0);
