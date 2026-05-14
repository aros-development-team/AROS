/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: checkbox.gadget class library init/expunge - opens bevel.image
          so the class is registered for NewObject lookups.
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "checkbox_intern.h"

static int Checkbox_InitLib(struct CheckBoxBase_intern *base)
{
    base->rc_BevelBase = OpenLibrary("images/bevel.image", 0);
    if (!base->rc_BevelBase)
        base->rc_BevelBase = OpenLibrary("bevel.image", 0);
    return TRUE;
}

static int Checkbox_ExpungeLib(struct CheckBoxBase_intern *base)
{
    if (base->rc_BevelBase)
    {
        CloseLibrary(base->rc_BevelBase);
        base->rc_BevelBase = NULL;
    }
    return TRUE;
}

ADD2INITLIB(Checkbox_InitLib, 0);
ADD2EXPUNGELIB(Checkbox_ExpungeLib, 0);
