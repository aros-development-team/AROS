/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Header file for intregions.c
    Lang: english
*/
#ifndef INTREGIONS_H
#define INTREGIONS_H

#include <graphics/gfxbase.h>
#include <graphics/regions.h>

/* CHECK_EVIL_RECTS:

   If 1, code will assume such rects to be like an empty region

   If 0, code reacts on such rects in a probably undefined matter
   and things like layers.library might freak out if an app for
   example tries to install a clip rectangle (region) with maxx < minx
   and/or maxy < miny. Causing apps to render outside of layer/window
   and possibly crash.
   
*/

#define CHECK_EVIL_RECTS 1
   
#if CHECK_EVIL_RECTS

#define IS_RECT_EVIL(rect)           \
(                                    \
    ((rect)->MaxX < (rect)->MinX) || \
    ((rect)->MaxY < (rect)->MinY)    \
)

#define ARE_COORDS_EVIL(minx,miny,maxx,maxy) \
(                                            \
    ((maxx) < (minx)) ||                     \
    ((maxy) < (miny))                        \
)

#else

#define IS_RECT_EVIL(rect)                   0
#define ARE_COORDS_EVIL(minx,miny,maxx,maxy) 0

#endif


BOOL clearrectrect(struct Rectangle* clearrect, struct Rectangle* rect,
		   struct RegionRectangle** erg);

#define Bounds(x)  (&(x)->bounds)
#define MinX(rr)   (Bounds(rr)->MinX)
#define MaxX(rr)   (Bounds(rr)->MaxX)
#define MinY(rr)   (Bounds(rr)->MinY)
#define MaxY(rr)   (Bounds(rr)->MaxY)
#define Width(rr)  (MaxX(rr) - MinX(rr) + 1)
#define Height(rr) (MaxY(rr) - MinY(rr) + 1)

#define _DoRectsOverlap(Rect, x1, y1, x2, y2) \
(                                             \
    y1 <= (Rect)->MaxY &&                     \
    y2 >= (Rect)->MinY &&                     \
    x1 <= (Rect)->MaxX &&                     \
    x2 >= (Rect)->MinX                        \
)

#define overlap(a,b) _DoRectsOverlap(&(a), (b).MinX, (b).MinY, (b).MaxX, (b).MaxY)

#define _AndRectRect(rect1, rect2, intersect)                  \
({                                                             \
    BOOL res;                                                  \
                                                               \
    if (overlap(*(rect1), *(rect2)))                           \
    {                                                          \
	(intersect)->MinX = MAX((rect1)->MinX, (rect2)->MinX); \
 	(intersect)->MinY = MAX((rect1)->MinY, (rect2)->MinY); \
	(intersect)->MaxX = MIN((rect1)->MaxX, (rect2)->MaxX); \
 	(intersect)->MaxY = MIN((rect1)->MaxY, (rect2)->MaxY); \
                                                               \
	res = TRUE;                                            \
    }                                                          \
    else                                                       \
        res = FALSE;                                           \
                                                               \
    res;                                                       \
})

#define _AreRectsEqual(Rect1, Rect2)  \
(                                     \
    (Rect1)->MinX == (Rect2)->MinX && \
    (Rect1)->MinY == (Rect2)->MinY && \
    (Rect1)->MaxX == (Rect2)->MaxX && \
    (Rect1)->MaxY == (Rect2)->MaxY    \
)

/* Checks whether the rectangle (x1, y1)-(x2, y2) is completely contained in Rect */
#define _IsRectInRect(Rect, x1, y1, x2, y2) \
(                                           \
    y1 >= (Rect)->MinY &&                   \
    y2 <= (Rect)->MaxY &&                   \
    x1 >= (Rect)->MinX &&                   \
    x2 <= (Rect)->MaxX                      \
)

#define _IsPointInRect(Rect, x, y) _IsRectInRect(Rect, (x), (y), (x), (y))

#define _SwapRegions(region1, region2) \
do                                     \
{                                      \
    struct Region tmp;                 \
                                       \
    tmp      = *region1;               \
    *region1 = *region2;               \
    *region2 = tmp;                    \
} while (0);

#define _TranslateRect(rect, dx, dy) \
do                                   \
{                                    \
    struct Rectangle *_rect = rect;  \
    WORD _dx = dx;                   \
    WORD _dy = dy;                   \
    (_rect)->MinX += _dx;            \
    (_rect)->MinY += _dy;            \
    (_rect)->MaxX += _dx;            \
    (_rect)->MaxY += _dy;            \
} while(0)

#define _TranslateRegionRectangles(rr, dx, dy) \
if (dx || dy)                                  \
{                                              \
    struct RegionRectangle *_rr;               \
                                               \
    for (_rr = rr; _rr; _rr = _rr->Next)       \
        _TranslateRect(&_rr->bounds, dx, dy);  \
}

/*
 ** Important: ClearRegion calls InitRegion(). If you change something here
 ** which should not be done in case of ClearRegion() then don't forget to
 ** fix ClearRegion!
 */

#define InitRegion(region)            \
do                                    \
{                                     \
    MinX(region) = 0;                 \
    MinY(region) = 0;                 \
    MaxX(region) = 0;                 \
    MaxY(region) = 0;                 \
    (region)->RegionRectangle = NULL; \
} while (0)

/* Copy RegionRectangles from R1 to R2. R2 will be overwritten!!! */
static inline BOOL _CopyRegionRectangles(struct Region *R1, struct Region *R2, struct GfxBase *GfxBase)
{
    if (_LinkRegionRectangleList(R1->RegionRectangle, &R2->RegionRectangle, GfxBase))
    {
    	R2->RegionRectangle = Head(R2->RegionRectangle);
        R2->bounds = R1->bounds;
        return TRUE;
    }
    return FALSE;
}

/* ugly hack, I know... */
#ifndef GfxBase

typedef BOOL BandOperation
(
    LONG                     OffX1,
    LONG                     OffX2,
    LONG                     MinY,
    LONG                     MaxY,
    struct RegionRectangle  *Src1,
    struct RegionRectangle  *Src2,
    struct RegionRectangle **DstPtr,
    struct RegionRectangle **NextSrc1Ptr,
    struct RegionRectangle **NextSrc2Ptr,
    struct GfxBase          *GfxBase
);

BOOL _DoOperationBandBand
(
    BandOperation           *Operation,
    LONG                     OffX1,
    LONG                     OffX2,
    LONG 		     OffY1,
    LONG 		     OffY2,
    struct RegionRectangle  *Src1,
    struct RegionRectangle  *Src2,
    struct RegionRectangle **DstPtr,
    struct Rectangle        *DstBounds,
    struct GfxBase          *GfxBase
);

extern BandOperation _OrBandBand, _AndBandBand, _ClearBandBand;

#endif

#endif /* !INTREGIONS_H */
