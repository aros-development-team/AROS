/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction bevel.image - Internal definitions
*/

#ifndef BEVEL_INTERN_H
#define BEVEL_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/imageclass.h>
#include <images/bevel.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct BevelBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

struct BevelData
{
    ULONG           bd_Style;           /* Bevel style (BVS_*) */
    STRPTR          bd_Label;           /* Optional label string */
    ULONG           bd_LabelPlace;      /* Label placement */
    UWORD           bd_TextPen;         /* Text pen */
    UWORD           bd_FillPen;         /* Fill pen */
    UWORD           bd_FillTextPen;     /* Fill text pen */
};

#endif /* BEVEL_INTERN_H */
