/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _BITMAP_H
#define _BITMAP_H

/* This attribute interface is common for both x11 onscreen and offscreen bitmap
   classes, although they don't have a common superclass
*/


#define IID_Hidd_X11BitMap "hidd.bitmap.x11bitmap"

#define HiddX11BitMapAB __abHidd_X11BitMap

extern OOP_AttrBase HiddX11BitMapAB;

enum
{
    aoHidd_X11BitMap_Drawable,
    aoHidd_X11BitMap_MasterWindow,
    num_Hidd_X11BitMap_Attrs
};

#define aHidd_X11BitMap_Drawable	(HiddX11BitMapAB + aoHidd_X11BitMap_Drawable)
#define aHidd_X11BitMap_MasterWindow	(HiddX11BitMapAB + aoHidd_X11BitMap_MasterWindow)


/* This structure is used for both onscreen and offscreen X11 bitmaps !! */

#define GetSysDisplay()     	    (data->display)
#define GetSysScreen()      	    (data->screen)
#define GetSysCursor()      	    (data->cursor)

#define IS_BM_ATTR(attr, idx) 	    ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_X11BM_ATTR(attr, idx)    ( ( (idx) = (attr) - HiddX11BitMapAB) < num_Hidd_X11BitMap_Attrs)


/* This structure is used as instance data for both the
   onbitmap and offbitmap classes.
*/

struct bitmap_data
{
    union
    {
    	Window 	xwindow;
	Pixmap  pixmap;
    } 	    	    drawable;
    Window	    masterxwindow;    
    Cursor	    cursor;
    unsigned long   sysplanemask;
    Colormap	    colmap;
    GC 		    gc;	/* !!! This is an X11 GC, NOT a HIDD gc */
    Display	    *display;
    int		    screen;
    int		    flags;    
};

#define BMDF_COLORMAP_ALLOCED 1

#endif /* _BITMAP_H */
