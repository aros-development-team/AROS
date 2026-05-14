/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: textfield.gadget class library init/expunge - opens bevel.image.
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "textfield_intern.h"

static int TextField_InitLib(struct TextFieldBase_intern *base)
{
    base->rc_BevelBase = OpenLibrary("images/bevel.image", 0);
    if (!base->rc_BevelBase)
        base->rc_BevelBase = OpenLibrary("bevel.image", 0);
    return TRUE;
}

static int TextField_ExpungeLib(struct TextFieldBase_intern *base)
{
    if (base->rc_BevelBase)
    {
        CloseLibrary(base->rc_BevelBase);
        base->rc_BevelBase = NULL;
    }
    return TRUE;
}

ADD2INITLIB(TextField_InitLib, 0);
ADD2EXPUNGELIB(TextField_ExpungeLib, 0);
