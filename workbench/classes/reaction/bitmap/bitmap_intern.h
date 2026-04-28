/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction bitmap.image - Internal definitions
*/

#ifndef BITMAP_INTERN_H
#define BITMAP_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <images/bitmap.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

struct BitmapData
{
    struct BitMap   *bd_BitMap;          /* Normal state bitmap */
    UBYTE           *bd_MaskPlane;       /* Normal mask plane */
    UWORD           bd_Width;            /* Source bitmap width */
    UWORD           bd_Height;           /* Source bitmap height */
    STRPTR          bd_SourceFile;       /* Filename to load */
    struct Screen   *bd_Screen;          /* Screen for color mapping */
    ULONG           bd_Precision;        /* Remapping precision */
    BOOL            bd_Masking;          /* Use mask plane */
    BOOL            bd_Transparent;      /* Transparent mode */
    WORD            bd_OffsetX;          /* Source X offset */
    WORD            bd_OffsetY;          /* Source Y offset */
    struct BitMap   *bd_SelectBitMap;    /* Selected state bitmap */
    UBYTE           *bd_SelectMaskPlane; /* Selected mask plane */
    struct BitMap   *bd_DisBitMap;       /* Disabled state bitmap */
    UBYTE           *bd_DisMaskPlane;    /* Disabled mask plane */
};

#endif /* BITMAP_INTERN_H */
