/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _UAEGFXBITMAP_H
#define _UAEGFXBITMAP_H

#define IID_Hidd_UAEGFXBitMap "hidd.bitmap.uaegfxbitmap"

#include "uaertg.h"

/* This structure is used as instance data for the bitmap class. */

struct bm_data
{
    struct MinNode node;
    OOP_Object	    	*pixfmtobj;	/* Cached pixelformat object */
    OOP_Object	    	*gfxhidd;	/* Cached driver object */
    ULONG rgbformat;
    UBYTE *VideoData;
    ULONG memsize;
    BOOL invram;
    WORD width, height, align;
    WORD disp_width, disp_height;
    WORD bytesperpixel;
    WORD bytesperline;
    UBYTE *palette;
    WORD topedge, leftedge;
    WORD locked;
};

#endif
