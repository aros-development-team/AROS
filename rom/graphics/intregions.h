/*
    (C) 1995-97 AROS - The Amiga Research OS
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
{                                              \
    struct RegionRectangle *_rr;               \
                                               \
    for (_rr = rr; _rr; _rr = _rr->Next)       \
        _TranslateRect(&_rr->bounds, dx, dy);  \
}

#define USE_BANDED_FUNCTIONS 1

/* ugly hack, I know... */
#ifndef GfxBase

BOOL _OrRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
);

BOOL _XorRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
);

BOOL _ClearRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
);

BOOL _AndRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
);

BOOL _OrRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
);

BOOL _AndRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
);

BOOL _ClearRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
);

BOOL _XorRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
);

BOOL _AreRegionsEqual
(
    struct Region *R1,
    struct Region *R2
);

void _NormalizeRegion
(
    struct Region  *R,
    struct GfxBase *GfxBase
);


#endif

#endif /* !INTREGIONS_H */
