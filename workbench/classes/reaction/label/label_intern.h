/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction label.image - Internal definitions
*/

#ifndef LABEL_INTERN_H
#define LABEL_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/imageclass.h>
#include <images/label.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct LabelBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

struct LabelData
{
    STRPTR          ld_Text;            /* Label text */
    Object         *ld_Image;           /* Optional image object */
    ULONG           ld_Justification;   /* LJ_LEFT, LJ_CENTER, LJ_RIGHT */
    ULONG           ld_SoftStyle;       /* Text style flags */
    BOOL            ld_DisposeImage;    /* Dispose image on OM_DISPOSE */
    LONG           *ld_Mapping;         /* Pen mapping */
    struct DrawInfo *ld_DrawInfo;       /* DrawInfo */
    UBYTE           ld_Underscore;      /* Underscore character */
};

#endif /* LABEL_INTERN_H */
