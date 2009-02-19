/*
    Copyright  1995-2008, The AROS Development Team. All rights reserved.
    $Id: bitmap.h 23833 2005-12-20 14:41:50Z stegerg $
*/

#ifndef _BITMAP_H
#define _BITMAP_H

/* This attribute interface is common for both gdi onscreen and offscreen bitmap
   classes, although they don't have a common superclass
*/


#define IID_Hidd_GDIBitMap "hidd.bitmap.gdibitmap"

#define HiddGDIBitMapAB __abHidd_GDIBitMap

/* extern OOP_AttrBase HiddGDIBitMapAB; */

enum
{
    aoHidd_GDIBitMap_DeviceContext,
    aoHidd_GDIBitMap_Window,
    num_Hidd_GDIBitMap_Attrs
};

#define aHidd_GDIBitMap_DeviceContext	(HiddGDIBitMapAB + aoHidd_GDIBitMap_DeviceContext)
#define aHidd_GDIBitMap_Window		(HiddGDIBitMapAB + aoHidd_GDIBitMap_Window)


/* This structure is used for both onscreen and offscreen GDI bitmaps !! */

#define GetSysDisplay()     	    (data->display)
#define GetSysScreen()      	    (data->screen)
#define GetSysCursor()      	    (data->cursor)

#define IS_BM_ATTR(attr, idx) 	    ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_GDIBM_ATTR(attr, idx)    ( ( (idx) = (attr) - HiddGDIBitMapAB) < num_Hidd_GDIBitMap_Attrs)


/* This structure is used as instance data for the bitmap class.
*/

struct bitmap_data
{
    APTR dc;
    APTR bitmap;
    APTR dc_bitmap;
/*  Cursor	    cursor; */
    APTR display;
    APTR window;
    int	 flags;    
};

#define BMDF_COLORMAP_ALLOCED 1

#endif /* _BITMAP_H */
