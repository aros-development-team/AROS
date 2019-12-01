/*
    Copyright  1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _AMIGABITMAP_H
#define _AMIGABITMAP_H

#include "amigavideo_intern.h"

#define IID_Hidd_BitMap_AmigaVideo "hidd.bitmap.amigavideo"

enum
{
    aoHidd_BitMap_AmigaVideo_Drawable,
    aoHidd_BitMap_AmigaVideo_Compositor,

    num_Hidd_BitMap_AmigaVideo_Attrs
};

#define aHidd_BitMap_AmigaVideo_Drawable    (__IHidd_BitMap_AmigaVideo + aoHidd_BitMap_AmigaVideo_Drawable)
#define aHidd_BitMap_AmigaVideo_Compositor  (__IHidd_BitMap_AmigaVideo + aoHidd_BitMap_AmigaVideo_Compositor)

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - __IHidd_Attr) < num_Hidd_BitMap_Attrs)
#define IS_AmigaVideoBM_ATTR(attr, idx) ( ( (idx) = (attr) - __IHidd_BitMap_AmigaVideo) < num_Hidd_BitMap_AmigaVideo_Attrs)


/* This structure is used as instance data for the bitmap class.
*/

struct amigabm_data
{
    /* display composition data ... */
    struct Node                 node;
    struct copper2data          copper2;
    struct copper2data          copper2i;
    IPTR                        modeid;
    
    UBYTE                       res; // 0 = lores, 1 = hires, 2 = shres
    UBYTE                       interlace;
    WORD                        modulopre, modulo;
    UWORD                       ddfstrt, ddfstop;
    UWORD                       use_colors;

    UBYTE                       *palette;
    OOP_Object                  *compositor;
    UBYTE                       bploffsets[8];

    /* old stuff.. */
    struct BitMap               *pbm;
    WORD                        width;
    WORD                        height;
    WORD                        bytesperrow;
    UBYTE                       depth;
    UBYTE                       planebuf_size;
    WORD                        topedge, leftedge;
    WORD                        align;
    WORD                        displaywidth;
    WORD                        displayheight;
    /* pixel read/write cache */
    ULONG                       pixelcacheoffset;
    UBYTE                       pixelcache[32];
    ULONG                       writemask;
    /* flags */
    BOOL                        disp;               /* displayable ? */
    BOOL                        vis;                /* visible ? */
};

#include "chipset.h"

#endif /* _AMIGABITMAP_H */
