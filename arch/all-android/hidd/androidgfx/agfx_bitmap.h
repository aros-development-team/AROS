/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android bitmap class definitions
    Lang: English.
*/

#define AO(x) (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)
#define IS_BM_ATTR(attr, idx) (((idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)

struct bitmap_data
{
    LONG     bm_left;		/* Physical coordinates of top-left corner */
    LONG     bm_top;
    ULONG    win_width;		/* Display window size			   */
    ULONG    win_height;
    ULONG    bm_width;		/* Bitmap size				   */
    ULONG    bm_height;
    ULONG    mod;		/* Bytes per line			   */
    IPTR     pixels;		/* Address in memory			   */
    BOOL     visible;
};
