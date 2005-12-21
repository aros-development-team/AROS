/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
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

#define IID_Hidd_VesaGfxBitMap "hidd.bitmap.vesabitmap"

#define HiddVesaGfxBitMapAttrBase __abHidd_VesaGfxBitMap

/* extern OOP_AttrBase HiddVesaGfxBitMapAttrBase; */

enum
{
    aoHidd_VesaGfxBitMap_Drawable,
    num_Hidd_VesaGfxBitMap_Attrs
};

#define aHidd_VesaGfxBitMap_Drawable	(HiddVesaGfxBitMapAttrBase + aoHidd_VesaGfxBitMap_Drawable)



/* This structure is used for both onscreen and offscreen VGA bitmaps !! */

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_VesaGfxBM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddVesaGfxBitMapAttrBase) < num_Hidd_VesaGfxBitMap_Attrs)


/*
   This structure is used as instance data for both the
   onbitmap and offbitmap classes.
*/

struct HWRegs
{
    UBYTE clt[768];
};

struct HWData;

struct BitmapData
{
    struct HWRegs   	regs;
    struct HWData	*data;
    UBYTE   	    	*VideoData;	/* Pointing to video data */
    ULONG   	    	width;      	/* Width of bitmap */
    ULONG   	    	height;		/* Height of bitmap */
    UBYTE   	    	bytesperpix;
    ULONG   	    	bytesperline;
    ULONG   	    	cmap[16];   	/* ColorMap */
    BYTE    	    	bpp;         	/* 8 -> chunky; planar otherwise */
    BYTE    	    	disp;        	/* !=0 - displayable */
    struct MouseData 	*mouse;
};

struct Box
{
    int x1, y1;
    int x2, y2;
};

#endif /* _BITMAP_H */
