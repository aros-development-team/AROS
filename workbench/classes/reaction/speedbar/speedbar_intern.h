/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction speedbar.gadget - Internal definitions
*/

#ifndef SPEEDBAR_INTERN_H
#define SPEEDBAR_INTERN_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/speedbar.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#define G(obj)  ((struct Gadget *)(obj))

struct SpeedBarData
{
    struct List     *sd_Buttons;        /* List of SpeedBarNode buttons */
    ULONG           sd_Orientation;     /* Horizontal or vertical */
    ULONG           sd_BevelStyle;      /* Bevel style for buttons */
    struct Window   *sd_Window;         /* Parent window */
    BOOL            sd_EvenSize;        /* Force even button sizing */
    BOOL            sd_RaisedButtons;   /* Draw raised button frames */
    BOOL            sd_SmallImages;     /* Use small images */
};

#endif /* SPEEDBAR_INTERN_H */
