/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _BITMAP_H
#define _BITMAP_H

#define IID_Hidd_GDIBitMap "hidd.bitmap.gdibitmap"

#define HiddGDIBitMapAB __abHidd_GDIBitMap

enum
{
    aoHidd_GDIBitMap_DeviceContext, /* ..G - bitmap's device context          */
    aoHidd_GDIBitMap_SysDisplay,    /* I.. - a display to be compatible with  */

    num_Hidd_GDIBitMap_Attrs
};

#define aHidd_GDIBitMap_DeviceContext   (HiddGDIBitMapAB + aoHidd_GDIBitMap_DeviceContext)
#define aHidd_GDIBitMap_Window          (HiddGDIBitMapAB + aoHidd_GDIBitMap_Window)
#define aHidd_GDIBitMap_SysDisplay      (HiddGDIBitMapAB + aoHidd_GDIBitMap_SysDisplay)

#define IS_BM_ATTR(attr, idx)       ( ( (idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)
#define IS_GDIBM_ATTR(attr, idx)    ( ( (idx) = (attr) - HiddGDIBitMapAB) < num_Hidd_GDIBitMap_Attrs)

/* This structure is used as instance data for the bitmap class.
*/

struct bitmap_data
{
    struct MinNode node;
    APTR  dc;         /* Device context                                                          */
    APTR  bitmap;     /* Actual Windows bitmap object                                            */
    APTR  dc_bitmap;  /* Original DC's bitmap object, needs to be put back before freeing the DC */
    APTR  display;    /* System display - to what DC should be compatible                        */
    ULONG window;     /* Window in which the bitmap is displayed                                 */
    LONG  win_width;  /* Window size (cached from ModeID)                                        */
    LONG  win_height;
    LONG  bm_width;   /* Requested bitmap size. (not rounded up)                                 */
    LONG  bm_height;
    LONG  bm_left;    /* Bitmap edge coordinates                                                 */
    LONG  bm_top;
};

#endif /* _BITMAP_H */
