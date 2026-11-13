/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.
*/

#ifndef HeadlessGFX_BITMAP_H
#define HeadlessGFX_BITMAP_H

#include <hidd/gfx.h>

#define CLID_Hidd_BitMap_Headless        "hidd.bitmap.headless"
#define IID_Hidd_BitMap_Headless         "hidd.bitmap.headless"

#define IS_BM_ATTR(attr, idx) ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)

/*
   This structure is used as instance data for the bitmap class.
*/
struct HeadlessGfxBitMapData
{
    BYTE    	    	disp;        	/* !=0 - displayable */
};

#endif /* HeadlessGFX_BITMAP_H */
