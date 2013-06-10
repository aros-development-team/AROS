/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _AMIGABITMAP_H
#define _AMIGABITMAP_H

#define IID_Hidd_AmigaVideoBitMap "hidd.bitmap.amigavideobitmap"

enum
{
    aoHidd_AmigaVideoBitMap_Drawable,
    num_Hidd_AmigaVideoBitMap_Attrs
};

#define aHidd_AmigaVideoBitMap_Drawable	(__IHidd_AmigaVideoBitmap + aoHidd_AmigaVideoBitMap_Drawable)

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - __IHidd_Attr) < num_Hidd_BitMap_Attrs)
#define IS_AmigaVideoBM_ATTR(attr, idx) ( ( (idx) = (attr) - __IHidd_AmigaVideoBitmap) < num_Hidd_AmigaVideoBitMap_Attrs)


/* This structure is used as instance data for the bitmap class.
*/

struct amigabm_data
{
    struct MinNode node;
    UBYTE *planes[8];
    WORD width;
    WORD height;
    WORD bytesperrow;
    UBYTE depth;
    UBYTE planebuf_size;
    WORD topedge, leftedge;
    BOOL disp;
    WORD align;
    WORD displaywidth;
    WORD displayheight;
    /* pixel read/write cache */
    ULONG pixelcacheoffset;
    UBYTE pixelcache[32];
    ULONG writemask;
};

#include "chipset.h"

#endif /* _AMIGABITMAP_H */
