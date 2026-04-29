/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction slider.gadget - Internal definitions
*/

#ifndef SLIDER_INTERN_H
#define SLIDER_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/slider.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct SliderBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

struct SliderData
{
    LONG            sd_Min;             /* Minimum value */
    LONG            sd_Max;             /* Maximum value */
    LONG            sd_Level;           /* Current value */
    ULONG           sd_Orientation;     /* SLIDER_HORIZONTAL or SLIDER_VERTICAL */
    UWORD           sd_Ticks;           /* Number of major tick marks */
    UWORD           sd_ShortTicks;      /* Number of minor tick marks between major ticks */
    BOOL            sd_Invert;          /* Invert direction */
    STRPTR          sd_LevelFormat;     /* printf-style format for level display */
    ULONG           sd_LevelPlace;      /* Placement of level text (PLACETEXT_xxx) */
    UWORD           sd_LevelMaxLen;     /* Maximum length of level text */
};

#endif /* SLIDER_INTERN_H */
