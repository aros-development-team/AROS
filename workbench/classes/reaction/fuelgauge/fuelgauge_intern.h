/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction fuelgauge.gadget - Internal definitions
*/

#ifndef FUELGAUGE_INTERN_H
#define FUELGAUGE_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/fuelgauge.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#define G(obj)  ((struct Gadget *)(obj))

struct FuelGaugeData
{
    LONG            fgd_Level;          /* Current level value */
    LONG            fgd_Min;            /* Minimum value */
    LONG            fgd_Max;            /* Maximum value */
    ULONG           fgd_Orientation;    /* Horizontal or vertical */
    UWORD           fgd_Ticks;          /* Number of major tick marks */
    UWORD           fgd_ShortTicks;     /* Number of minor tick marks */
    BOOL            fgd_Percent;        /* Show percentage text */
    ULONG           fgd_Justification;  /* Text justification */
};

#endif /* FUELGAUGE_INTERN_H */
