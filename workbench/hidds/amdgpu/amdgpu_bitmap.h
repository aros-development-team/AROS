/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _AMDGPU_BITMAP_H
#define _AMDGPU_BITMAP_H

#include <hidd/gfx.h>

#include "amdgpu_kms.h"

/* This attribute interface is common for both onscreen and offscreen bitmap
   classes, although they don't have a common superclass */

#define IID_Hidd_AmdgpuBitMap "hidd.bitmap.amdgpu"

#define HiddAmdgpuBitMapAttrBase __abHidd_AmdgpuBitMap

enum {
	aoHidd_AmdgpuBitMap_Drawable,
	num_Hidd_AmdgpuBitMap_Attrs
};

#define aHidd_AmdgpuBitMap_Drawable	(HiddAmdgpuBitMapAttrBase + aoHidd_AmdgpuBitMap_Drawable)

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_AmdgpuBM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddAmdgpuBitMapAttrBase) < num_Hidd_AmdgpuBitMap_Attrs)

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
    struct Amdgpu_FB fb;
    drmModeModeInfoPtr  mode;
};

#define LOCK_BITMAP                 { ObtainSemaphore(&data->bmsem); }
#define UNLOCK_BITMAP               { ReleaseSemaphore(&data->bmsem); }

#endif /* _AMDGPU_BITMAP_H */
