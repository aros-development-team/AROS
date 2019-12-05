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
    struct CopList              *bmcl;
    struct copper2data          copld;
    struct copper2data          copsd;
    IPTR                        modeid;
    
    UBYTE                       res; // 0 = lores, 1 = hires, 2 = shres
    UBYTE                       interlace;
    WORD                        modulopre, modulo;
    UWORD                       ddfstrt, ddfstop;
    UWORD                       use_colors;

    UWORD                       bplcon3;

    UBYTE                       *palette;
    OOP_Object                  *compositor;
    UBYTE                       bploffsets[8];

#if USE_FAST_BMPOSCHANGE
    OOP_MethodFunc         bmposchange;
    OOP_Class 	          *bmposchange_Class;
#endif

    /* old stuff.. */
    struct BitMap               *pbm;
    WORD                        width;
    WORD                        height;
    WORD                        bytesperrow;
    UBYTE                       depth;
    UBYTE                       planebuf_size;
    WORD                        topedge, leftedge;
    WORD                        updtop, updleft;
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

#define BMDATFROMCOPLD(x)    ((struct amigabm_data *)((IPTR)x - (offsetof(struct amigabm_data,copld))))
#define BMDATFROMCOPSD(x)    ((struct amigabm_data *)((IPTR)x - (offsetof(struct amigabm_data,copsd))))

#include "chipset.h"

#endif /* _AMIGABITMAP_H */
