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
    num_Hidd_VideoCoreGfxBitMap_Attrs
};

#define aHidd_VideoCoreGfxBitMap_Drawable	(HiddVideoCoreGfxBitMapAttrBase + aoHidd_VideoCoreGfxBitMap_Drawable)

/* Scan-out bitmap pixel formats besides the screens' own 32bpp one:
 * 16bpp little-endian, and 32bpp in B,G,R,X byte order on an HVS whose
 * screens are R,G,B,X (HVS6) */
#define VCGFX_PX_RGB565   (1 << 1)
#define VCGFX_PX_RGB555   (1 << 2)
#define VCGFX_PX_BGRA     (1 << 8)

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

/* Buffers of a scan-out bitmap (aHidd_BitMap_Scanout); screens use one */
#define VCGFX_SCAN_BUFFERS  3

/* A page of scanout memory (vcgfx_scanout_alloc), 4K aligned. */
struct vcgfx_scanbuf
{
    APTR                sb_Raw;
    ULONG               sb_RawSize;
    UBYTE              *sb_Pixels;
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
	struct vcgfx_scanbuf scan[VCGFX_SCAN_BUFFERS]; /* Pages on scanout, [front] = VideoData */
	UBYTE               front;
	UBYTE               back;                   /* Scan-out bitmap: the buffer to fill next */
	UBYTE               nbufs;                  /* Scan-out bitmap: its buffers, 0 = a screen */
	ULONG               pxflags;                /* VCGFX_PX_*, 0 = the screens' 32bpp */
	UBYTE               videofmt;               /* vHidd_VideoFmt_NV12/I420, 0 = RGB */
	UBYTE               colorspace;             /* vHidd_ColorSpace_* of a video bitmap */
	ULONG               planeoff[2];            /* Video: chroma plane offsets in a page */
	ULONG               cpitch;                 /* Video: chroma bytes per row */
	ULONG               modew, modeh;           /* Screen: its mode's display size */
};

#endif /* _VIDEOCOREGFX_BITMAP_H */
