/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _BITMAP_H
#define _BITMAP_H

#include "displayhw.h"
#include <hidd/graphics.h>

/*
   This attribute interface is common for both vga onscreen and offscreen bitmap
   classes, although they don't have a common superclass
*/

#define IID_Hidd_DisplayBitMap "hidd.bitmap.displaybitmap"

#define HiddDisplayBitMapAB __abHidd_DisplayBitMap
extern OOP_AttrBase HiddDisplayBitMapAB;

enum {
    aoHidd_DisplayBitMap_Drawable,
    
    num_Hidd_DisplayBitMap_Attrs
};

#define aHidd_DisplayBitMap_Drawable	(HiddDisplayBitMapAB + aoHidd_DisplayBitMap_Drawable)



/* This structure is used for both onscreen and offscreen Display bitmaps !! */

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_DisplayBM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddDisplayBitMapAB) < num_Hidd_DisplayBitMap_Attrs)


/*
   This structure is used as instance data for both the
   onbitmap and offbitmap classes.
*/

struct bitmap_data
{
    struct displayHWRec	*Regs;		/* Registers */
    unsigned char	*VideoData;	/* Pointing to video data */
    unsigned long	width;		/* Width of bitmap */
    unsigned long	height;		/* Height of bitmap */
    unsigned long	cmap[16];	/* ColorMap */
    char		bpp;    	/* 8 -> chunky; planar otherwise */
    char		disp;		/* !=0 - displayable */
};

struct Box
{
    int x1, y1;
    int x2, y2;
};

VOID bitmap_clear(OOP_Class *, OOP_Object *, struct pHidd_BitMap_Clear *);
BOOL bitmap_setcolors(OOP_Class *, OOP_Object *, struct pHidd_BitMap_SetColors *);
VOID bitmap_putpixel(OOP_Class *, OOP_Object *, struct pHidd_BitMap_PutPixel *);
HIDDT_Pixel bitmap_getpixel(OOP_Class *, OOP_Object *, struct pHidd_BitMap_GetPixel *);
ULONG bitmap_drawpixel(OOP_Class *, OOP_Object *, struct pHidd_BitMap_DrawPixel *);

void displayRefreshArea(struct bitmap_data *, int , struct Box *);

#endif /* _BITMAP_H */
