/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: button.gadget class library init/expunge — opens dependent
          classes once per process so they are registered with Intuition
          for any NewObject(NULL, "<classname>", ...) the gadget makes.
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "button_intern.h"

static int Button_InitLib(struct ButtonBase_intern *base)
{
    base->rc_BevelBase = OpenLibrary("images/bevel.image", 0);
    if (!base->rc_BevelBase)
        base->rc_BevelBase = OpenLibrary("bevel.image", 0);

    /* Failing to open is non-fatal: GM_RENDER will simply skip the bevel
     * if NewObject can't find the class. */
    return TRUE;
}

static int Button_ExpungeLib(struct ButtonBase_intern *base)
{
    if (base->rc_BevelBase)
    {
        CloseLibrary(base->rc_BevelBase);
        base->rc_BevelBase = NULL;
    }
    return TRUE;
}

ADD2INITLIB(Button_InitLib, 0);
ADD2EXPUNGELIB(Button_ExpungeLib, 0);
