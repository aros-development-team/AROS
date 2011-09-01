/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 bitmap class, external definitions
    Lang: english
*/

#ifndef _BITMAP_H
#define _BITMAP_H

/*
 * This attribute interface is common for both x11 onscreen and offscreen bitmap
 * classes, although they don't have a common superclass
 */

#define IID_Hidd_X11BitMap "hidd.bitmap.x11bitmap"

extern OOP_AttrBase HiddX11BitMapAB;

/* extern OOP_AttrBase HiddX11BitMapAB; */

enum
{
    aoHidd_X11BitMap_Drawable,
    aoHidd_X11BitMap_SysDisplay,
    aoHidd_X11BitMap_SysScreen,
    aoHidd_X11BitMap_GC,
    aoHidd_X11BitMap_SysCursor,
    aoHidd_X11BitMap_ColorMap,
    aoHidd_X11BitMap_VisualClass, /* stegerg */

    num_Hidd_X11BitMap_Attrs
};

#define aHidd_X11BitMap_Drawable	(HiddX11BitMapAB + aoHidd_X11BitMap_Drawable)
#define aHidd_X11BitMap_SysDisplay	(HiddX11BitMapAB + aoHidd_X11BitMap_SysDisplay)
#define aHidd_X11BitMap_SysScreen	(HiddX11BitMapAB + aoHidd_X11BitMap_SysScreen)
#define aHidd_X11BitMap_GC		(HiddX11BitMapAB + aoHidd_X11BitMap_GC)
#define aHidd_X11BitMap_SysCursor	(HiddX11BitMapAB + aoHidd_X11BitMap_SysCursor)
#define aHidd_X11BitMap_ColorMap	(HiddX11BitMapAB + aoHidd_X11BitMap_ColorMap)
#define aHidd_X11BitMap_VisualClass	(HiddX11BitMapAB + aoHidd_X11BitMap_VisualClass) /* stegerg */

#define IS_BM_ATTR(attr, idx) 	    ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_X11BM_ATTR(attr, idx)    ( ( (idx) = (attr) - HiddX11BitMapAB) < num_Hidd_X11BitMap_Attrs)

#endif
