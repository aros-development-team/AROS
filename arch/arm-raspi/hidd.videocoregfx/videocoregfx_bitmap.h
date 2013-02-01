/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _VIDEOCORE_BITMAP_H
#define _VIDEOCORE_BITMAP_H

#include <hidd/graphics.h>
#include "videocore_mouse.h"

/* This attribute interface is common for both onscreen and offscreen bitmap
   classes, although they don't share a common superclass */

#define IID_Hidd_VideoCoreGfxBitMap             "hidd.bitmap.videocore"


enum {
    aoHidd_VideoCoreGfxBitMap_Drawable,
    num_Hidd_VideoCoreGfxBitMap_Attrs
};

#define aHidd_VideoCoreGfxBitMap_Drawable	(HiddVideoCoreGfxBitMapAttrBase + aoHidd_VideoCoreGfxBitMap_Drawable)

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

struct HWRegs {
    UBYTE clt[768];
};

/* Only include videocore_hardware.h now so that struct Box is known */

#include "videocore_hardware.h"

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

#endif /* _VIDEOCORE_BITMAP_H */
