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

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct PenMapBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

struct PenMapData
{
    UBYTE          *pd_SelectData;      /* Selected state render data */
    struct Screen  *pd_Screen;          /* Screen for pen allocation */
    ULONG           pd_Precision;       /* Color matching precision */
    WORD            pd_SelectBGPen;     /* Selected background pen */
    APTR            pd_Palette;         /* Palette data */
    struct ColorMap *pd_ColorMap;       /* Colormap for remap */
    UWORD           pd_ImageType;       /* Image type */
    UWORD           pd_Transparent;     /* Transparent pen index */
    BOOL            pd_MaskBlit;        /* Use blitmask for transparency */
};

#endif /* PENMAP_INTERN_H */
