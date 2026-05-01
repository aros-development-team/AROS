/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction palette.gadget - Internal definitions
*/

#ifndef PALETTE_INTERN_H
#define PALETTE_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/palette.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct PaletteBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

struct PaletteGadData
{
    ULONG           pd_Color;           /* Selected color index */
    ULONG           pd_ColorOffset;     /* First color in palette range */
    ULONG           pd_NumColors;       /* Number of colors to display */
};

#endif /* PALETTE_INTERN_H */
