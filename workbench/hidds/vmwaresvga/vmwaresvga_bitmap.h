/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _VMWARESVGA_BITMAP_H
#define _VMWARESVGA_BITMAP_H

#include <hidd/gfx.h>

#include "vmwaresvga_kms.h"

/* This attribute interface is common for both onscreen and offscreen bitmap
   classes, although they don't have a common superclass */

#define IID_Hidd_VMWareSVGABitMap "hidd.bitmap.vmwaresvga"

#define HiddVMWareSVGABitMapAttrBase __abHidd_VMWareGfxBitMap

enum {
	aoHidd_VMWareSVGABitMap_Drawable,
	num_Hidd_VMWareSVGABitMap_Attrs
};

#define aHidd_VMWareSVGABitMap_Drawable	(HiddVMWareSVGABitMapAttrBase + aoHidd_VMWareSVGABitMap_Drawable)

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_VMWareSVGABM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddVMWareSVGABitMapAttrBase) < num_Hidd_VMWareSVGABitMap_Attrs)

/* This structure is used as instance data for both the
   onbitmap and offbitmap classes. */

struct BitmapData {
    struct SignalSemaphore bmsem;

    UBYTE               *VideoData;             /* Pointing to video data */
    ULONG               width;                  /* Width of bitmap */
    ULONG               height;                 /* Height of bitmap */
    ULONG               pitch;                  /* Bytes per line */
    UBYTE               bytesperpix;
    ULONG               cmap[16];               /* ColorMap */
    BYTE                bpp;                    /* 8 -> chunky; planar otherwise */
    BYTE                disp;                   /* !=0 - displayable */

    /* Onscreen bitmaps: the scanout buffer and the mode it is shown in */
    struct VMWareSVGA_FB fb;
    drmModeModeInfoPtr  mode;
};

#define LOCK_BITMAP                 { ObtainSemaphore(&data->bmsem); }
#define UNLOCK_BITMAP               { ReleaseSemaphore(&data->bmsem); }

#endif /* _VMWARESVGA_BITMAP_H */
