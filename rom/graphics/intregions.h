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


BOOL clearrectrect(struct Rectangle* clearrect, struct Rectangle* rect,
		   struct RegionRectangle** erg);

#define Bounds(x) (&(x)->bounds)
#define MinX(rr)   (Bounds(rr)->MinX)
#define MaxX(rr)   (Bounds(rr)->MaxX)
#define MinY(rr)   (Bounds(rr)->MinY)
#define MaxY(rr)   (Bounds(rr)->MaxY)

#define overlapX(a,b)         \
(                             \
    ((a).MinX <= (b).MaxX) && \
    ((a).MaxX >= (b).MinX)    \
)

#define overlapY(a,b)         \
(                             \
    ((a).MinY <= (b).MaxY) && \
    ((a).MaxY >= (b).MinY)    \
)

#define overlap(a,b) (overlapY(a,b) && overlapX(a,b))


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

#define _IsPointInRect(Rect, x, y) \
(                                  \
    y >= (Rect)->MinY &&           \
    y <= (Rect)->MaxY &&           \
    x >= (Rect)->MinX &&           \
    x <= (Rect)->MaxX              \
)

#define _TranslateRect(rect, dx, dy) \
{                                    \
    struct Rectangle *_rect = rect;  \
    WORD _dx = dx;                   \
    WORD _dy = dy;                   \
    (_rect)->MinX += _dx;            \
    (_rect)->MinY += _dy;            \
    (_rect)->MaxX += _dx;            \
    (_rect)->MaxY += _dy;            \
}

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
{                                     \
    MinX(region) = 0;                 \
    MinY(region) = 0;                 \
    MaxX(region) = 0;                 \
    MaxY(region) = 0;                 \
    (region)->RegionRectangle = NULL; \
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
