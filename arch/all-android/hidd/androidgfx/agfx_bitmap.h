/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android bitmap class definitions
    Lang: English.
*/

#define AO(x) (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)
#define IS_BM_ATTR(attr, idx) (((idx) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)

struct bitmap_data
{
    LONG  bm_left;
    LONG  bm_top;
    ULONG win_width;
    ULONG win_height;
    ULONG bm_width;
    ULONG bm_height;
    ULONG *data;
};
