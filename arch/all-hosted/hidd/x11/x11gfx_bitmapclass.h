/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: X11 bitmap class, external definitions
*/

#ifndef _BITMAP_H
#define _BITMAP_H

/*
 * This attribute interface is common for both x11 onscreen and offscreen bitmap
 * classes, although they don't have a common superclass
 */

extern OOP_AttrBase HiddX11BitMapAB;

/* extern OOP_AttrBase HiddX11BitMapAB; */

#define CLID_Hidd_BitMap_X11    "hidd.bitmap.x11"
#define IID_Hidd_BitMap_X11     "hidd.bitmap.x11"

enum
{
    aoHidd_BitMap_X11_Drawable,
    aoHidd_BitMap_X11_SysDisplay,
    aoHidd_BitMap_X11_SysScreen,
    aoHidd_BitMap_X11_GC,
    aoHidd_BitMap_X11_SysCursor,
    aoHidd_BitMap_X11_ColorMap,
    aoHidd_BitMap_X11_VisualClass, /* stegerg */

    num_Hidd_BitMap_X11_Attrs
};

#define aHidd_BitMap_X11_Drawable	(HiddX11BitMapAB + aoHidd_BitMap_X11_Drawable)
#define aHidd_BitMap_X11_SysDisplay	(HiddX11BitMapAB + aoHidd_BitMap_X11_SysDisplay)
#define aHidd_BitMap_X11_SysScreen	(HiddX11BitMapAB + aoHidd_BitMap_X11_SysScreen)
#define aHidd_BitMap_X11_GC		(HiddX11BitMapAB + aoHidd_BitMap_X11_GC)
#define aHidd_BitMap_X11_SysCursor	(HiddX11BitMapAB + aoHidd_BitMap_X11_SysCursor)
#define aHidd_BitMap_X11_ColorMap	(HiddX11BitMapAB + aoHidd_BitMap_X11_ColorMap)
#define aHidd_BitMap_X11_VisualClass	(HiddX11BitMapAB + aoHidd_BitMap_X11_VisualClass) /* stegerg */

#define IS_BM_ATTR(attr, idx) 	    ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_X11BM_ATTR(attr, idx)    ( ( (idx) = (attr) - HiddX11BitMapAB) < num_Hidd_BitMap_X11_Attrs)

#endif
