/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction penmap.image - Internal definitions
*/

#ifndef PENMAP_INTERN_H
#define PENMAP_INTERN_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <images/penmap.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

struct PenMapData
{
    UBYTE          *pd_PenMap;          /* Normal state pen-indexed pixel data */
    UWORD           pd_Width;           /* Penmap width in pixels */
    UWORD           pd_Height;          /* Penmap height in pixels */
    struct Screen  *pd_Screen;          /* Screen for pen allocation */
    ULONG           pd_Precision;       /* Color matching precision */
    UBYTE          *pd_SelectPenMap;    /* Selected state penmap */
    UBYTE          *pd_DisPenMap;       /* Disabled state penmap */
    UBYTE           pd_Transparent;     /* Transparent pen index */
    UWORD           pd_NumColors;       /* Number of colors in palette */
    ULONG          *pd_RGBData;         /* RGB color data array */
};

#endif /* PENMAP_INTERN_H */
