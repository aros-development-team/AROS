/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction getfont.gadget - Internal definitions
*/

#ifndef GETFONT_INTERN_H
#define GETFONT_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <graphics/text.h>
#include <gadgets/getfont.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct GetFontBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

struct GetFontData
{
    STRPTR          gfd_TitleText;      /* Requester title */
    struct TextAttr *gfd_TextAttr;      /* Current font text attributes */
    BOOL            gfd_DoStyle;        /* Show style options */
    BOOL            gfd_FixedWidthOnly; /* Only show fixed-width fonts */
    UWORD           gfd_MinHeight;      /* Minimum font height */
    UWORD           gfd_MaxHeight;      /* Maximum font height */
};

#endif /* GETFONT_INTERN_H */
