/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: layout.gadget class library init/expunge — opens dependent
          classes once per process so they are registered with Intuition
          for any NewObject(NULL, "<classname>", ...) the gadget makes.
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "layout_intern.h"

static int Layout_InitLib(struct LayoutBase_intern *base)
{
    base->rc_BevelBase = OpenLibrary("images/bevel.image", 0);
    if (!base->rc_BevelBase)
        base->rc_BevelBase = OpenLibrary("bevel.image", 0);
    return TRUE;
}

static int Layout_ExpungeLib(struct LayoutBase_intern *base)
{
    if (base->rc_BevelBase)
    {
        CloseLibrary(base->rc_BevelBase);
        base->rc_BevelBase = NULL;
    }
    if (base->rc_PageBase)
    {
        CloseLibrary(base->rc_PageBase);
        base->rc_PageBase = NULL;
    }
    return TRUE;
}

ADD2INITLIB(Layout_InitLib, 0);
ADD2EXPUNGELIB(Layout_ExpungeLib, 0);
