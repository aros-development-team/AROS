/*
    Copyright � 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _BITMAP_H
#define _BITMAP_H

#include <hidd/graphics.h>
#include "mouse.h"
#include "hardware.h"
/*
   This attribute interface is common for both vga onscreen and offscreen bitmap
   classes, although they don't have a common superclass
*/

#define IID_Hidd_VMWareGfxBitMap "hidd.bitmap.vmwarebitmap"

#define HiddVMWareGfxBitMapAttrBase __abHidd_VMWareGfxBitMap
extern OOP_AttrBase HiddVMWareGfxBitMapAttrBase;

enum {
	aoHidd_VMWareGfxBitMap_Drawable,
	num_Hidd_VMWareGfxBitMap_Attrs
};

#define aHidd_VMWareGfxBitMap_Drawable	(HiddVMWareGfxBitMapAttrBase + aoHidd_VMWareGfxBitMap_Drawable)



/* This structure is used for both onscreen and offscreen VGA bitmaps !! */

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_VMWareGfxBM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddVMWareGfxBitMapAttrBase) < num_Hidd_VMWareGfxBitMap_Attrs)


/*
   This structure is used as instance data for both the
   onbitmap and offbitmap classes.
*/

struct HWRegs {
	UBYTE clt[768];
};

struct BitmapData {
	struct HWRegs  regs;
	struct HWData	*data;
	UBYTE *VideoData;	/* Pointing to video data */
	ULONG width;      /* Width of bitmap */
	ULONG height;		/* Height of bitmap */
	UBYTE bytesperpix;
	ULONG cmap[16];   /* ColorMap */
	BYTE bpp;         /* 8 -> chunky; planar otherwise */
	BYTE disp;        /* !=0 - displayable */
	struct MouseData *mouse;
};

struct Box
{
    int x1, y1;
    int x2, y2;
};

#if 0
VOID bitmap_clear(OOP_Class *, OOP_Object *, struct pHidd_BitMap_Clear *);
BOOL bitmap_setcolors(OOP_Class *, OOP_Object *, struct pHidd_BitMap_SetColors *);
VOID bitmap_putpixel(OOP_Class *, OOP_Object *, struct pHidd_BitMap_PutPixel *);
HIDDT_Pixel bitmap_getpixel(OOP_Class *, OOP_Object *, struct pHidd_BitMap_GetPixel *);
ULONG bitmap_drawpixel(OOP_Class *, OOP_Object *, struct pHidd_BitMap_DrawPixel *);
#endif

#endif /* _BITMAP_H */
