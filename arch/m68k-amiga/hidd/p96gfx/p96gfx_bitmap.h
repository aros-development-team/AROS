/*
    Copyright  1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef P96GFX_BITMAP_H
#define P96GFX_BITMAP_H

#define IID_Hidd_BitMap_P96     "hidd.bitmap.p96gfx"

#include "p96gfx_rtg.h"

/* This structure is used as instance data for the bitmap class. */

struct P96GfxBitMapData
{
    struct MinNode              node;
    struct SignalSemaphore      bmLock;
    OOP_Object                  *pixfmtobj;	/* Cached pixelformat object */
    OOP_Object                  *gfxhidd;	/* Cached driver object */
    ULONG                       rgbformat;
    UBYTE                       *VideoData;
    ULONG                       memsize;
    BOOL                        invram;
    WORD                        width, height, align;
    WORD                        bytesperpixel;
    WORD                        bytesperline;
    UBYTE                       *palette;
    WORD                        topedge, leftedge;
    WORD                        locked;
    struct p96gfx_carddata      *gfxCardData;
};

#define LOCK_BITMAP(data)       {ObtainSemaphore(&(data)->bmLock);}
#define TRYLOCK_BITMAP(data)    (AttemptSemaphore(&(data)->bmLock))
#define UNLOCK_BITMAP(data)     {ReleaseSemaphore(&(data)->bmLock);}

#endif /* P96GFX_BITMAP_H */
