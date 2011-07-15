#ifndef _BITMAP_H
#define _BITMAP_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/graphics.h>

#define IID_Hidd_VesaGfxBitMap "hidd.bitmap.vesabitmap"

enum
{
    aoHidd_VesaGfxBitMap_Drawable,
    aoHidd_VesaGfxBitMap_CompositingHidd,
    num_Hidd_VesaGfxBitMap_Attrs
};

#define aHidd_VesaGfxBitMap_Drawable	(HiddVesaGfxBitMapAttrBase + aoHidd_VesaGfxBitMap_Drawable)
#define aHidd_VesaGfxBitMap_CompositingHidd	(HiddVesaGfxBitMapAttrBase + aoHidd_VesaGfxBitMap_CompositingHidd)

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_VesaGfxBM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddVesaGfxBitMapAttrBase) < num_Hidd_VesaGfxBitMap_Attrs)

/*
   This structure is used as instance data for the bitmap class.
*/
struct BitmapData
{
    UBYTE   	    	*VideoData;	/* Pointing to video data */
    LONG   	    	height;		/* Height of bitmap */
    UBYTE   	    	bytesperpix;
    ULONG   	    	bytesperline;
    UBYTE *   	    	DAC;   		/* Hardware palette registers */
    BYTE    	    	bpp;         	/* Cached bits per pixel */
    BYTE    	    	disp;        	/* !=0 - displayable */
    OOP_Object	    	*pixfmtobj;	/* Cached pixelformat object */
    OOP_Object	    	*gfxhidd;	/* Cached driver object */
    OOP_Object	    	*compositing;	/* Cached compositing object */
    LONG		disp_width;	/* Display size */
    LONG		disp_height;
    LONG		xoffset;	/* Bitmap offset */
    LONG		yoffset;
};

#endif /* _BITMAP_H */
