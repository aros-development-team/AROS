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
    aoHidd_GDIBitMap_Drawable,
    aoHidd_GDIBitMap_MasterWindow,
    num_Hidd_GDIBitMap_Attrs
};

#define aHidd_GDIBitMap_Drawable	(HiddGDIBitMapAB + aoHidd_GDIBitMap_Drawable)
#define aHidd_GDIBitMap_MasterWindow	(HiddGDIBitMapAB + aoHidd_GDIBitMap_MasterWindow)


/* This structure is used for both onscreen and offscreen GDI bitmaps !! */

#define GetSysDisplay()     	    (data->display)
#define GetSysScreen()      	    (data->screen)
#define GetSysCursor()      	    (data->cursor)

#define IS_BM_ATTR(attr, idx) 	    ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_GDIBM_ATTR(attr, idx)    ( ( (idx) = (attr) - HiddGDIBitMapAB) < num_Hidd_GDIBitMap_Attrs)


/* This structure is used as instance data for both the
   onbitmap and offbitmap classes.
*/

struct bitmap_data
{
    APTR drawable;
    APTR bitmap;
    APTR dc_bitmap;
/*  Window	    masterxwindow;    
    Cursor	    cursor;
    unsigned long   sysplanemask;
    Colormap	    colmap;
    GC 		    gc;	** !!! This is an GDI GC, NOT a HIDD gc */
    APTR	    display;
    int		    flags;    
};

#define BMDF_COLORMAP_ALLOCED 1

#endif /* _BITMAP_H */
