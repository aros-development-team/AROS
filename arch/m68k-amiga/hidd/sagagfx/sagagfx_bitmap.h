#ifndef SAGAGFX_BITMAP_H
#define SAGAGFX_BITMAP_H

/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SAGAGfx header.
    Lang: English.
*/

#include "sagagfx_hidd.h"
#include "sagagfx_hw.h"

#include <hidd/gfx.h>

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)

struct SAGARegs {
    ULONG       pixelclock;
    UWORD       video_mode;
    UWORD       hpixel;
    UWORD       hsstart;
    UWORD       hsstop;
    UWORD       htotal;
    UWORD       vpixel;
    UWORD       vsstart;
    UWORD       vsstop;
    UWORD       vtotal;
    UWORD       hvsync;
    UWORD       modulo;
    IPTR        memptr;
};

/*
   This structure is used as instance data for the bitmap class.
*/
struct SAGAGfxBitmapData
{
    UBYTE       *VideoBuffer;   /* Start of framebuffer, as allocated */
    UBYTE       *VideoData;	    /* Start of aligned framebuffer */
    LONG        width;          /* Bitmap size */
    LONG        height;
    UBYTE       bytesperpix;
    UBYTE       bitsperpix;
    ULONG       bytesperline;
    ULONG *     CLUT;           /* Hardware palette registers */
    struct SAGARegs hwregs;     /* Hardware registers */
    BYTE        bpp;            /* Cached bits per pixel */
    BYTE        disp;           /* !=0 - displayable */
    OOP_Object  *pixfmtobj;     /* Cached pixelformat object */
    OOP_Object  *gfxhidd;       /* Cached driver object */
    LONG        xoffset;        /* Bitmap offset */
    LONG        yoffset;
};

#endif /* SAGAGFX_BITMAP_H */
