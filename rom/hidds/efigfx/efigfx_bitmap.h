/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef EFIGFX_BITMAP_H
#define EFIGFX_BITMAP_H

#include <hidd/gfx.h>

#define CLID_Hidd_BitMap_EFI        "hidd.bitmap.efi"
#define IID_Hidd_BitMap_EFI         "hidd.bitmap.efi"

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)

/*
   This structure is used as instance data for the bitmap class.
*/
struct EFIGfxBitMapData
{
    UBYTE   	    	*VideoData;	/* Pointing to video data */
    LONG		width;		/* Bitmap size */
    LONG   	    	height;
    UBYTE   	    	bytesperpix;
    ULONG   	    	bytesperline;
    UBYTE *   	    	DAC;   		/* Hardware palette registers */
    BYTE    	    	bpp;         	/* Cached bits per pixel */
    BYTE    	    	disp;        	/* !=0 - displayable */
    OOP_Object	    	*pixfmtobj;	/* Cached pixelformat object */
    OOP_Object	    	*displayhidd;	/* display object */
    LONG		disp_width;	/* Display size */
    LONG		disp_height;
    LONG		xoffset;	/* Bitmap offset */
    LONG		yoffset;
};

#endif /* EFIGFX_BITMAP_H */
