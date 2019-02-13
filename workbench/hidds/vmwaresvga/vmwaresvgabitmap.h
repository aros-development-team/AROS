/*
    Copyright � 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _VMWARESVGA_BITMAP_H
#define _VMWARESVGA_BITMAP_H

#include <hidd/gfx.h>
#include "vmwaresvgamouse.h"

/* This attribute interface is common for both vga onscreen and offscreen bitmap
   classes, although they don't have a common superclass */

#define IID_Hidd_VMWareSVGABitMap "hidd.bitmap.vmwaresvga"

#define HiddVMWareSVGABitMapAttrBase __abHidd_VMWareGfxBitMap
/* extern OOP_AttrBase HiddVMWareSVGABitMapAttrBase; */

enum {
	aoHidd_VMWareSVGABitMap_Drawable,
	num_Hidd_VMWareSVGABitMap_Attrs
};

#define aHidd_VMWareSVGABitMap_Drawable	(HiddVMWareSVGABitMapAttrBase + aoHidd_VMWareSVGABitMap_Drawable)

/* This structure is used for both onscreen and offscreen VGA bitmaps !! */

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_VMWareSVGABM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddVMWareSVGABitMapAttrBase) < num_Hidd_VMWareSVGABitMap_Attrs)

/* This structure is used as instance data for both the
   onbitmap and offbitmap classes. */

struct Box
{
    int x1, y1;
    int x2, y2;
};

struct HWRegs {
	UBYTE clt[768];
};

struct BitmapData {
	struct HWRegs       regs;
	struct HWData       *data;
	UBYTE               *VideoData;             /* Pointing to video data */
	ULONG               width;                  /* Width of bitmap */
	ULONG               height;                 /* Height of bitmap */
	UBYTE               bytesperpix;
	ULONG               cmap[16];               /* ColorMap */
	BYTE                bpp;                    /* 8 -> chunky; planar otherwise */
	BYTE                disp;                   /* !=0 - displayable */
	struct MouseData    *mouse;
};

/* Only include vmwaresvgahardware.h now so that struct Box is known */

#include "vmwaresvgahardware.h"

#endif /* _VMWARESVGA_BITMAP_H */
