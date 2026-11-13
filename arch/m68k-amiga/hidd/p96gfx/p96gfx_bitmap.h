/*
    Copyright  1995-2026, The AROS Development Team. All rights reserved.
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

/*
 * The per pixel size memory methods of the bitmap class (CopyMemBox8..32,
 * CopyLUTMemBox16..32, PutMemTemplate8..32, PutMemPattern8..32,
 * PutMem32Image8..24, GetMem32Image8..24, FillMemRect24..32) are numbered
 * consecutively and take the same message within a family, so the stub of
 * the first one reaches the others when its method base is advanced.
 * FillMemRect8 and 16 differ in the width of their fill value.
 */
#define P96_BPP_VALID(bpp)      (((bpp) >= 1) && ((bpp) <= 4))
#define P96_BPP_BASE(bpp)       (HiddBitMapBase + (bpp) - 1)

/* Check that a variant directly follows the previous one */
#define P96_ASSERT_NEXT_ID(prev, next)                                          \
    _Static_assert(moHidd_BitMap_##next == moHidd_BitMap_##prev + 1,            \
                   "moHidd_BitMap_" #next " must follow " #prev)
#define P96_ASSERT_NEXT_METHOD(prev, next)                                      \
    P96_ASSERT_NEXT_ID(prev, next);                                             \
    _Static_assert(sizeof(struct pHidd_BitMap_##next) ==                        \
                   sizeof(struct pHidd_BitMap_##prev),                          \
                   "pHidd_BitMap_" #next " must match " #prev)
#define P96_ASSERT_BPP_METHODS(name)                                            \
    P96_ASSERT_NEXT_METHOD(name##8, name##16);                                  \
    P96_ASSERT_NEXT_METHOD(name##16, name##24);                                 \
    P96_ASSERT_NEXT_METHOD(name##24, name##32)

#endif /* P96GFX_BITMAP_H */
