/*
    Copyright (C) 2013-2017, The AROS Development Team. All rights reserved.
*/

#ifndef _VIDEOCOREGFX_BITMAP_H
#define _VIDEOCOREGFX_BITMAP_H

#include <hidd/gfx.h>

/* This attribute interface is common for both onscreen and offscreen bitmap
   classes, although they don't share a common superclass */

#define IID_Hidd_BitMap_VideoCore4             "hidd.bitmap.bcmvc4"


enum {
    aoHidd_VideoCoreGfxBitMap_Drawable,
    aoHidd_VideoCoreGfxBitMap_BackDrawable,    /* [G..] back page phys addr, 0 = no flipping */
    aoHidd_VideoCoreGfxBitMap_Flip,            /* [.S.] set TRUE to flip front/back page */
    num_Hidd_VideoCoreGfxBitMap_Attrs
};

#define aHidd_VideoCoreGfxBitMap_Drawable	(HiddVideoCoreGfxBitMapAttrBase + aoHidd_VideoCoreGfxBitMap_Drawable)
#define aHidd_VideoCoreGfxBitMap_BackDrawable	(HiddVideoCoreGfxBitMapAttrBase + aoHidd_VideoCoreGfxBitMap_BackDrawable)
#define aHidd_VideoCoreGfxBitMap_Flip	(HiddVideoCoreGfxBitMapAttrBase + aoHidd_VideoCoreGfxBitMap_Flip)

/* This structure is used for both onscreen and offscreen bitmaps !! */

#define IS_BM_ATTR(attr, idx)                   ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_VideoCoreGfxBM_ATTR(attr, idx)       ( ( (idx) = (attr) - HiddVideoCoreGfxBitMapAttrBase) < num_Hidd_VideoCoreGfxBitMap_Attrs)

/* This structure is used as instance data for both the
   onbitmap and offbitmap classes. */

struct Box
{
    int x1, y1;
    int x2, y2;
};

struct BitmapData {
	APTR                data;
	UBYTE               *VideoData;             /* Pointing to video data */
	ULONG               width;                  /* Width of bitmap */
	ULONG               height;                 /* Height of bitmap */
	ULONG               bytesperrow;            /* Pitch in bytes (matches BitMap super) */
	UBYTE               bytesperpix;
	ULONG               cmap[16];               /* ColorMap */
	BYTE                bpp;                    /* 8 -> chunky; planar otherwise */
	BYTE                disp;                   /* !=0 - displayable */
	struct MouseData    *mouse;
	ULONG               gpuhandle;              /* Offscreen: firmware mem handle (0 = RAM-backed) */
	ULONG               dispwidth;              /* Mode width (width is 16px-aligned) */
};

#endif /* _VIDEOCOREGFX_BITMAP_H */
