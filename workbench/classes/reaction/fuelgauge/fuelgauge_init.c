/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: fuelgauge.gadget class library init/expunge - opens bevel.image.
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "fuelgauge_intern.h"

static int FuelGauge_InitLib(struct FuelGaugeBase_intern *base)
{
    base->rc_BevelBase = OpenLibrary("images/bevel.image", 0);
    if (!base->rc_BevelBase)
        base->rc_BevelBase = OpenLibrary("bevel.image", 0);
    return TRUE;
}

static int FuelGauge_ExpungeLib(struct FuelGaugeBase_intern *base)
{
    if (base->rc_BevelBase)
    {
        CloseLibrary(base->rc_BevelBase);
        base->rc_BevelBase = NULL;
    }
    return TRUE;
}

ADD2INITLIB(FuelGauge_InitLib, 0);
ADD2EXPUNGELIB(FuelGauge_ExpungeLib, 0);
