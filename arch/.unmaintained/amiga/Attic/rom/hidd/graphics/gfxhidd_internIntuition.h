#ifndef GRAPHICS_HIDD_INTERN_INTUITION_H
#define GRAPHICS_HIDD_INTERN_INTUITION_H
/*
    Copyright (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include file for amiga intuition graphics HIDD.
    Lang: english
*/

#include <hidd/graphics/gfxhidd_intern.h>

/* BitMap structure for intuition */
struct hGfx_bitMapInt
{
    struct hGfx_bitMap bm; /* default graphics hidd bitmap structure */
    struct BitMap *bitMap; /* pointer to a non displayble bitmap     */
    struct Screen *screen; /* pointer to a displayble bitmap an      */
                           /* intuition screen                       */
};


/* Graphics context structure for intuition */
struct hGfx_gcInt
{
    struct hGfx_gc  gc;        /* default graphics hidd graphics context */
                               /* structure                              */
    struct RastPort *rPort;    /* pointer to rastport for drawing        */
    struct RastPort *tmpRPort; /* tmp rastport eg. for fill operations   */
};

#endif /* GRAPHICS_HIDD_INTERN_INTUITION_H */
