/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _AMIGABITMAP_H
#define _AMIGABITMAP_H

#define IID_Hidd_AmigaVideoBitMap "hidd.bitmap.amigavideobitmap"

#define HiddAmigaVideoBitMapAB __abHidd_AmigaVideoBitMap

/* This structure is used as instance data for the bitmap class.
*/

struct planarbm_data
{
    struct MinNode node;
    UBYTE **planes; // bitmap pointers (aligned to 8-byte if AGA)
    UBYTE **planesmem; // allocated memory
    WORD width;
    WORD rows;
    WORD bytesperrow;
    UBYTE depth;
    UBYTE planes_alloced;
    UBYTE planebuf_size;
    WORD topedge, leftedge;
    BOOL disp;
};

#include "chipset.h"

#endif /* _AMIGABITMAP_H */
