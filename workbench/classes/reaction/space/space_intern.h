/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction space.gadget - Internal definitions
*/

#ifndef SPACE_INTERN_H
#define SPACE_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/space.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#define G(obj)  ((struct Gadget *)(obj))

struct SpaceData
{
    UWORD           sd_MinWidth;        /* Minimum width */
    UWORD           sd_MinHeight;       /* Minimum height */
    ULONG           sd_BevelStyle;      /* Bevel style */
    BOOL            sd_Transparent;     /* Transparent background */
};

#endif /* SPACE_INTERN_H */
