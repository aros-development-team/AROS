/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction button.gadget - Internal definitions
*/

#ifndef BUTTON_INTERN_H
#define BUTTON_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/button.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct ButtonBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

struct ButtonData
{
    Object          *bd_Glyph;          /* Glyph image object */
    STRPTR          bd_DomainString;    /* Domain sizing string */

    ULONG           bd_AutoButton;      /* Auto-button glyph type */
    ULONG           bd_BevelStyle;      /* Bevel style */
    ULONG           bd_Justification;   /* Text justification */
    ULONG           bd_SoftStyle;       /* Text style */

    UWORD           bd_TextPen;         /* Text pen */
    UWORD           bd_BackgroundPen;   /* Background pen */
    UWORD           bd_FillTextPen;     /* Fill text pen */
    UWORD           bd_FillPen;         /* Fill pen */

    BOOL            bd_PushButton;      /* Push button/toggle mode */
    BOOL            bd_Transparent;     /* Transparent background */
};

#endif /* BUTTON_INTERN_H */
