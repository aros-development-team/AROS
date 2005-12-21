/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _BITMAP_H
#define _BITMAP_H

#include "vgahw.h"
#include <hidd/graphics.h>

/*
   This attribute interface is common for both vga onscreen and offscreen bitmap
   classes, although they don't have a common superclass
*/

#define IID_Hidd_VGABitMap "hidd.bitmap.vgabitmap"

#define HiddVGABitMapAB __abHidd_VGABitMap
/* extern OOP_AttrBase HiddVGABitMapAB; */

enum {
    aoHidd_VGABitMap_Drawable,
    
    num_Hidd_VGABitMap_Attrs
};

#define aHidd_VGABitMap_Drawable	(HiddVGABitMapAB + aoHidd_VGABitMap_Drawable)



/* This structure is used for both onscreen and offscreen VGA bitmaps !! */

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_VGABM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddVGABitMapAB) < num_Hidd_VGABitMap_Attrs)


/*
   This structure is used as instance data for both the
   onbitmap and offbitmap classes.
*/

struct bitmap_data
{
    struct vgaHWRec	*Regs;		/* Registers */
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

void vgaRefreshArea(struct bitmap_data *, int , struct Box *);

#endif /* _BITMAP_H */
