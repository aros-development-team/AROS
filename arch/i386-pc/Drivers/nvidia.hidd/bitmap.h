/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _BITMAP_H
#define _BITMAP_H

#include "riva_hw.h"
#include <hidd/graphics.h>

/*
   This attribute interface is common for both vga onscreen and offscreen bitmap
   classes, although they don't have a common superclass
*/

#define IID_Hidd_NVBitMap "hidd.bitmap.nvbitmap"

#define HiddNVBitMapAttrBase __abHidd_NVBitMap
extern OOP_AttrBase HiddNVBitMapAttrBase;

enum {
    aoHidd_NVBitMap_Drawable,
    
    num_Hidd_NVBitMap_Attrs
};

#define aHidd_NVBitMap_Drawable	(HiddNVBitMapAttrBase + aoHidd_NVBitMap_Drawable)

/* This structure is used for both onscreen and offscreen VGA bitmaps !! */

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_NVBM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddNVBitMapAttrBase) < num_Hidd_NVBitMap_Attrs)

/*
   This structure is used as instance data for both the
   onbitmap and offbitmap classes.
*/

struct bitmap_data
{
    unsigned char	*VideoData;	/* Pointing to video data */
    unsigned long	width;		/* Width of bitmap */
    unsigned long	height;		/* Height of bitmap */
    char			bpp;    	/* 8 -> chunky; planar otherwise */
    char			disp;		/* !=0 - displayable */
	unsigned long	cmap[256];	/* ColorMap */
};

struct Box
{
    int x1, y1;
    int x2, y2;
};

#endif /* _BITMAP_H */
