/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction glyph.image - Internal definitions
*/

#ifndef GLYPH_INTERN_H
#define GLYPH_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/imageclass.h>
#include <images/glyph.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct GlyphBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

struct GlyphData
{
    ULONG           gd_Glyph;           /* Glyph type (GLYPH_*) */
};

#endif /* GLYPH_INTERN_H */
