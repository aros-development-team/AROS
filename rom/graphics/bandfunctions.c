#include <proto/graphics.h>

#include <clib/macros.h>

#include "graphics_intern.h"
#include "intregions.h"

#define DEBUG 0

#define Bounds(x) (&(x)->bounds)
#define MinX(rr)   (Bounds(rr)->MinX)
#define MaxX(rr)   (Bounds(rr)->MaxX)
#define MinY(rr)   (Bounds(rr)->MinY)
#define MaxY(rr)   (Bounds(rr)->MaxY)

#define ADVANCE(nextbandptr, rr)                    \
{                                                   \
    if ((rr)->Next && MinY((rr)->Next) == MinY(rr)) \
        rr = (rr)->Next;                            \
    else                                            \
    {                                               \
    	if (nextbandptr)                            \
            *nextbandptr = (rr)->Next;              \
        rr = NULL;                                  \
    }                                               \
}

#define LINK(prev, curr)     \
{                            \
    (curr)->Prev = prev;     \
    if (prev)                \
        (prev)->Next = curr; \
                             \
    (prev) = rr;             \
}                            \

#define NEWREG(first, rr)                  \
{                                          \
    rr = NewRegionRectangle();             \
    if (!rr)                               \
    {                                      \
        DisposeRegionRectangleList(first); \
        return FALSE;                      \
    }                                      \
                                           \
    if (!first)                            \
        first = rr;                        \
}

#define ADDRECT(first, prev, minx, maxx) \
{                                       \
   struct RegionRectangle *rr;          \
   NEWREG((first), rr);                 \
                                        \
   MinX(rr) = minx;                     \
   MinY(rr) = MinY;                     \
   MaxX(rr) = maxx;                     \
   MaxY(rr) = MaxY;                     \
                                        \
   LINK((prev), rr);                    \
}

#define ADDRECTMERGE(first, prev, minx, maxx)     \
{                                                 \
    if                                            \
    (                                             \
        !(prev) ||                                \
        ((minx-1) > MaxX(prev))                   \
    )                                             \
    {                                             \
	ADDRECT((first), (prev), (minx), (maxx)); \
    }                                             \
    else                                          \
    if (MaxX(prev) < maxx)                        \
    {                                             \
        MaxX(prev) = maxx;                        \
    }                                             \
}

/*
    "first" points to the first rectangle in the list of bands;
    "last"  points to the first rectangle in the latest band added to the list
    "band"  points to the first rectangle in the band being added to the list
    "band->Prev" points to the last rectangle in the band"
    "first->Prev" points to the rectangle that comes right before the first rectangle in the latest band added
*/
#define FIXLASTBAND(first, last)                      \
if (!first)                                           \
{                                                     \
    first = last;                                     \
}                                                     \
else                                                  \
{                                                     \
    struct RegionRectangle *newlastrect = last->Prev; \
                                                      \
    first->Prev->Next = last;                         \
    last->Prev = first->Prev;                         \
    first->Prev = newlastrect;                        \
}

#define ADDBAND(first, last, band)          \
if (band)                                   \
{                                           \
    if (!last)                              \
    {                                       \
	DstBounds->MinY = MinY(band);       \
        DstBounds->MinX = MinX(band);       \
        DstBounds->MaxX = MaxX(band->Prev); \
    }                                       \
    else                                    \
    {                                       \
        FIXLASTBAND(first, last)            \
    }                                       \
    last = band;                            \
                                            \
    if (DstBounds->MinX > MinX(last))       \
        DstBounds->MinX = MinX(last);       \
    if (DstBounds->MaxX < MaxX(last->Prev)) \
        DstBounds->MaxX = MaxX(last->Prev); \
}

#define ADDBANDMERGE(first, last, band)                                   \
{                                                                         \
    struct RegionRectangle *_last = (struct RegionRectangle *)~0;         \
    struct RegionRectangle *_band = (struct RegionRectangle *)~0;         \
                                                                          \
    if (band && last && MinY(band) == MaxY(last) + 1)                     \
    {                                                                     \
                                                                          \
	_band = band; _last = last;                                       \
	do                                                                \
        {                                                                 \
            if (MinX(_last) == MinX(_band) && MaxX(_last) == MaxX(_band)) \
            {                                                             \
                _last = _last->Next;                                      \
                _band = _band->Next;                                      \
            }                                                             \
            else                                                          \
                break;                                                    \
        } while (_last && _band);                                         \
                                                                          \
    }                                                                     \
                                                                          \
    if (_last == NULL && _band == NULL)                                   \
    {                                                                     \
        LONG NewMaxY = MaxY(band);                                        \
        for (_last = last; _last; _last = _last->Next)                    \
            MaxY(_last) = NewMaxY;                                        \
        DisposeRegionRectangleList(band);                                 \
    }                                                                     \
    else                                                                  \
    {                                                                     \
        ADDBAND(first, last, band);                                       \
    }                                                                     \
}

#if DOTEST
#    define DEBUG 1
#endif

#if DEBUG
void dumprect(struct Rectangle *rec)
{
    if (!rec)
    {
        kprintf("NULL\n");
        return;
    }

    kprintf("(%d,%d)-(%d,%d)\n", (int)rec->MinX, (int)rec->MinY,
                                 (int)rec->MaxX, (int)rec->MaxY);
}
void dumpregion(struct Region *reg)
{
    struct RegionRectangle *rr;

    if (!reg)
    {
        kprintf("NULL\n");
        return;
    }

    kprintf("Bounds: "); dumprect(&reg->bounds);

    for (rr = reg->RegionRectangle; rr; rr = rr->Next)
    {
        kprintf("    Rectangle %p: ", rr); dumprect(&rr->bounds);
    }
}

void dumpregionrectangles(struct RegionRectangle *rr)
{

    while (rr)
    {
	kprintf("%p (prev: %p - next: %p): ", rr, rr->Prev, rr->Next);
        dumprect(&rr->bounds);
        rr = rr->Next;
    }
}

void dumpband(struct RegionRectangle *rr, LONG OffX, LONG MinY, LONG MaxY)
{
    if (rr)
    {
	while (1)
        {
	    struct Rectangle r;
            r.MinX = MinX(rr) + OffX;
            r.MaxX = MaxX(rr) + OffX;
            r.MinY = MinY;
            r.MaxY = MaxY;
            kprintf("%p (prev: %p - next: %p): ", rr, rr->Prev, rr->Next);
            dumprect(&r);
            if (rr->Next && MinY(rr->Next) == MinY(rr)) rr = rr->Next; else break;
        }
    }
    else
        kprintf("\n");
}
#endif

BOOL _OrBandBand
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
)
{
    struct RegionRectangle *first = NULL, *last = NULL;

    while (Src1 && Src2)
    {
        if (MinX(Src1) + OffX1 < MinX(Src2) + OffX2)
        {
	    ADDRECTMERGE(first, last, MinX(Src1) + OffX1, MaxX(Src1) + OffX1);
            ADVANCE(NextSrc1Ptr, Src1);
        }
        else
        {
	    ADDRECTMERGE(first, last, MinX(Src2) + OffX2, MaxX(Src2) + OffX2);
            ADVANCE(NextSrc2Ptr, Src2);
        }
    }

    if (Src1)
    {
        do
        {
            ADDRECTMERGE(first, last, MinX(Src1) + OffX1, MaxX(Src1) + OffX1);
            ADVANCE(NextSrc1Ptr, Src1);
        } while (Src1);
    }
    else
    while (Src2)
    {
	ADDRECTMERGE(first, last, MinX(Src2) + OffX2, MaxX(Src2) + OffX2);
        ADVANCE(NextSrc2Ptr, Src2);
    }

    if (first)
        first->Prev = last;

    *DstPtr = first;

    return TRUE;
}

BOOL _AndBandBand
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
)
{
    struct RegionRectangle *first = NULL, *last = NULL;

    while (Src1 && Src2)
    {
        if (MinX(Src1) + OffX1 < MinX(Src2) + OffX2)
        {
	    if (MaxX(Src1) + OffX1 >= MaxX(Src2) + OffX2)
            {
            	/* Src1 totally covers Src2 */
                ADDRECT(first, last, MinX(Src2) + OffX2, MaxX(Src2) + OffX2);
 	        ADVANCE(NextSrc2Ptr, Src2);
            }
            else
            {
                if (MaxX(Src1) + OffX1 >= MinX(Src2) + OffX2)
            	    /* Src1 partially covers Src2 */
                    ADDRECT(first, last, MinX(Src2) + OffX2, MaxX(Src1) + OffX1);

                ADVANCE(NextSrc1Ptr, Src1);
            }
        }
	else
        {
	    if (MaxX(Src2) + OffX2 >= MaxX(Src1) + OffX1)
            {
            	/* Src2 totally covers Src1 */
                ADDRECT(first, last, MinX(Src1) + OffX1, MaxX(Src1) + OffX1);
 	        ADVANCE(NextSrc1Ptr, Src1);
            }
            else
            {
                if (MaxX(Src2) + OffX2 >= MinX(Src1) + OffX1)
            	    /* Src2 partially covers Src1 */
                    ADDRECT(first, last, MinX(Src1) + OffX1, MaxX(Src2) + OffX2);

                ADVANCE(NextSrc2Ptr, Src2);
            }
        }
    }

    if (Src1)
    {
        do ADVANCE(NextSrc1Ptr, Src1) while (Src1);
    }
    else
    while (Src2) ADVANCE(NextSrc2Ptr, Src2);

    if (first)
        first->Prev = last;

    *DstPtr = first;

    return TRUE;
}

BOOL _ClearBandBand
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
)
{
    struct RegionRectangle *first = NULL, *last = NULL;
    LONG MinX = 0;

    if (Src2)
        MinX = MinX(Src2) + OffX2;

    while (Src1 && Src2)
    {
	if (MaxX(Src1) + OffX1 < MinX)
        {
	    /* Subtrahend doesn't overlap minuend. Just skip it */
            ADVANCE(NextSrc1Ptr, Src1);
        }
        else
        if (MinX(Src1) + OffX1 <= MinX)
        {
            /* Subtrahend precedes minuend: nuke left edge of minuend */
	    MinX = MaxX(Src1) + OffX1 + 1;

            if (MinX > MaxX(Src2) + OffX2)
            {
		/*
                   Subtrahend completely overlaps minuend, so advance
                   to the next minuend and reset MinX to its left
   	        */
                ADVANCE(NextSrc2Ptr, Src2);
                if (Src2)
                    MinX = MinX(Src2) + OffX2;
            }
            else
            {
                /* Subtrahend doesn't extend beyond minuend, so advence to the next one */
                ADVANCE(NextSrc1Ptr, Src1);
            }
  	}
        else
        if (MinX(Src1) + OffX1 <= MaxX(Src2) + OffX2)
        {
	    /*
               Subtrahend covers part of minuend.
               Add uncovered part of minuend to the band and jump to the next
               subtrahend
     	    */

            ADDRECT(first, last, MinX, MinX(Src1) + OffX1 - 1);

            MinX = MaxX(Src1) + OffX1 + 1;

            if (MinX > MaxX(Src2) + OffX2)
            {
                /*Minuend used up: advance to the next one */
                ADVANCE(NextSrc2Ptr, Src2);
                if (Src2)
                    MinX = MinX(Src2) + OffX2;
            }
            else
            {
                /* Subtrahend used up */
                ADVANCE(NextSrc1Ptr, Src1);
            }
	}
        else
        {
	    /*
	       Minuend used up: add any remaining piece before advancing.
	     */

             if (MaxX(Src2) + OffX2 >= MinX)
             {
                 ADDRECT(first, last, MinX, MaxX(Src2) + OffX2);
             }

             ADVANCE(NextSrc2Ptr, Src2);
             if (Src2)
                 MinX = MinX(Src2) + OffX2;
        }
    }

    if (Src1)
    {
        do ADVANCE(NextSrc1Ptr, Src1) while (Src1);
    }
    else
    while (Src2)
    {
	ADDRECT(first, last, MinX, MaxX(Src2) + OffX2);
        ADVANCE(NextSrc2Ptr, Src2);
        if (Src2)
        {
            MinX = MinX(Src2) + OffX2;
        }
    }

    if (first)
        first->Prev = last;

    *DstPtr = first;

    return TRUE;
}

BOOL _XorBandBand
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
)
{
    struct RegionRectangle *Diff1 = NULL, *Diff2 = NULL;

    BOOL res = FALSE;

    if (_AndBandBand(OffX1, OffX2, MinY, MaxY, Src1, Src2, &Diff1, NULL, NULL, GfxBase))
    {
        if (_OrBandBand(OffX1, OffX2, MinY, MaxY, Src1, Src2, &Diff2, NextSrc1Ptr, NextSrc2Ptr, GfxBase))
	{
             res = _ClearBandBand(0, 0, MinY, MaxY, Diff1, Diff2, DstPtr, NULL, NULL, GfxBase);
        }
    }

    DisposeRegionRectangleList(Diff1);
    DisposeRegionRectangleList(Diff2);

    return res;
}

typedef BOOL (*BandOperation)
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
    BandOperation            Operation,
    LONG                     OffX1,
    LONG                     OffX2,
    LONG 		     OffY1,
    LONG 		     OffY2,
    struct RegionRectangle  *Src1,
    struct RegionRectangle  *Src2,
    struct RegionRectangle **DstPtr,
    struct Rectangle        *DstBounds,
    struct GfxBase          *GfxBase
)
{
    struct RegionRectangle *Dst;
    struct RegionRectangle *first = NULL, *last = NULL;
    struct RegionRectangle **NextSrc1Ptr = (void *)~0;
    struct RegionRectangle **NextSrc2Ptr = (void *)~0;
    struct RegionRectangle *Band1 = Src1, *Band2 = Src2;

    LONG TopY1 = 0, TopY2 = 0;

    while (Src1 && Src2)
    {
	LONG MinY, MaxY;

        if (NextSrc1Ptr)
        {
            TopY1 = MinY(Src1) + OffY1;
            NextSrc1Ptr = NULL;
        }

	if (NextSrc2Ptr)
        {
            TopY2 = MinY(Src2) + OffY2;
            NextSrc2Ptr = NULL;
        }

	if (TopY1 < TopY2)
        {
	    MinY = TopY1;
	    MaxY = MIN(MaxY(Src1) + OffY1, TopY2 - 1);
	    TopY1 = MaxY + 1;

            Band1 = Src1;
            Band2 = NULL;
        }
        else
        if (TopY2 < TopY1)
        {
	    MinY = TopY2;
	    MaxY = MIN(MaxY(Src2) + OffY2, TopY1 - 1);
	    TopY2 = MaxY + 1;

            Band1 = NULL;
            Band2 = Src2;
        }
        else
        {
	    MinY = TopY1;
	    MaxY = MIN(MaxY(Src1) + OffY1, MaxY(Src2) + OffY2);
	    TopY1 = TopY2 = MaxY + 1;

            Band1 = Src1;
            Band2 = Src2;
	}

	NextSrc1Ptr = (MaxY == MaxY(Src1) + OffY1) ? &Src1 : NULL;
	NextSrc2Ptr = (MaxY == MaxY(Src2) + OffY2) ? &Src2 : NULL;

        if (!Operation(OffX1, OffX2, MinY, MaxY, Band1, Band2, &Dst, NextSrc1Ptr, NextSrc2Ptr, GfxBase))
        {
            DisposeRegionRectangleList(first);
            DisposeRegionRectangleList(last);
            return FALSE;
        }

#if DEBUG
        {
	    kprintf(">>>NextSrc1Ptr: %p - %ld\n", NextSrc1Ptr, TopY1);
	    kprintf(">>>NextSrc2Ptr: %p - %ld\n", NextSrc2Ptr, TopY2);

            kprintf(">>>Band1: "); dumpband(Band1, OffX1, MinY, MaxY);
            kprintf(">>>Band2: "); dumpband(Band2, OffX2, MinY, MaxY);
            kprintf(">>>Dst: "); dumpband(Dst, 0, MinY, MaxY);
        }
#endif
	ADDBANDMERGE(first, last, Dst);
    }


    while (Src1)
    {
	if (NextSrc1Ptr)
            TopY1 = MinY(Src1) + OffY1;

        NextSrc1Ptr = (void *)~0;

        if (!Operation(OffX1, OffX2, TopY1, MaxY(Src1) + OffY1, Src1, NULL, &Dst, &Src1, NULL, GfxBase))
        {
            DisposeRegionRectangleList(first);
            DisposeRegionRectangleList(last);
            return FALSE;
        }

        ADDBANDMERGE(first, last, Dst);
    }

    while (Src2)
    {
	if (NextSrc2Ptr)
            TopY2 = MinY(Src2) + OffY2;

        NextSrc2Ptr = (void *)~0;

        if (!Operation(OffX1, OffX2, TopY2, MaxY(Src2) + OffY2, NULL, Src2, &Dst, NULL, &Src2, GfxBase))
        {
            DisposeRegionRectangleList(first);
            DisposeRegionRectangleList(last);
            return FALSE;
        }

        ADDBANDMERGE(first, last, Dst);
    }

    if (last)
    {
	FIXLASTBAND(first, last);
        first->Prev->Next = NULL;

	/*
          In certain conditions the Next pointer of the last rectangle in the region may
          point to a wrong location. This is an hacky but fast solution
        */
        first->Prev->Next = NULL;

        first->Prev = NULL;

        DstBounds->MaxY = MaxY(last);
    }

    *DstPtr = first;

    return TRUE;
}


BOOL _OrRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
)
{
    struct Region R3;

    if
    (
        _DoOperationBandBand
        (
            _OrBandBand,
            MinX(R1),
            MinX(R2),
	    MinY(R1),
            MinY(R2),
            R1->RegionRectangle,
            R2->RegionRectangle,
            &R3.RegionRectangle,
            &R3.bounds,
            GfxBase
        )
    )
    {
        DisposeRegionRectangleList(R2->RegionRectangle);

        *R2 = R3;

        _TranslateRegionRectangles(R3.RegionRectangle, -MinX(&R3), -MinY(&R3));

        return TRUE;
    }

    return FALSE;
}

BOOL _OrRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
)
{
    struct Region Res;
    struct RegionRectangle rr;

    rr.bounds = *Rect;
    rr.Next   = NULL;

    if
    (
        _DoOperationBandBand
        (
            _OrBandBand,
            MinX(Reg),
            0,
	    MinY(Reg),
            0,
            Reg->RegionRectangle,
            &rr,
            &Res.RegionRectangle,
            &Res.bounds,
            GfxBase
        )
    )
    {
        DisposeRegionRectangleList(Reg->RegionRectangle);

        *Reg = Res;

        _TranslateRegionRectangles(Res.RegionRectangle, -MinX(&Res), -MinY(&Res));

        return TRUE;
    }

    return FALSE;
}

BOOL _XorRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
)
{
    struct Region Res;
    struct RegionRectangle rr;

    rr.bounds = *Rect;
    rr.Next   = NULL;

    if
    (
        _DoOperationBandBand
        (
            _XorBandBand,
            MinX(Reg),
            0,
	    MinY(Reg),
            0,
            Reg->RegionRectangle,
            &rr,
            &Res.RegionRectangle,
            &Res.bounds,
            GfxBase
        )
    )
    {
        DisposeRegionRectangleList(Reg->RegionRectangle);

        *Reg = Res;

        _TranslateRegionRectangles(Res.RegionRectangle, -MinX(&Res), -MinY(&Res));

        return TRUE;
    }

    return FALSE;
}

BOOL _ClearRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
)
{
    struct Region Res;
    struct RegionRectangle rr;

    rr.bounds = *Rect;
    rr.Next   = NULL;

    if
    (
        _DoOperationBandBand
        (
            _ClearBandBand,
            0,
            MinX(Reg),
            0,
	    MinY(Reg),
            &rr,
            Reg->RegionRectangle,
            &Res.RegionRectangle,
            &Res.bounds,
            GfxBase
        )
    )
    {
        DisposeRegionRectangleList(Reg->RegionRectangle);

        *Reg = Res;

        _TranslateRegionRectangles(Res.RegionRectangle, -MinX(&Res), -MinY(&Res));

        return TRUE;
    }

    return FALSE;
}

#define DisposeBand(_Band, NextBandPtr)       \
{                                             \
    struct RegionRectangle *Band = _Band;     \
                                              \
    while (Band)                              \
    {                                         \
        struct RegionRectangle *OldRR = Band; \
        ADVANCE(NextBandPtr, Band);           \
        DisposeRegionRectangle(OldRR);        \
    }                                         \
}

static void __inline__ ShrinkBand
(
    struct RegionRectangle  *Band,
    LONG MinX,
    LONG MinY,
    LONG MaxX,
    LONG MaxY,
    struct RegionRectangle **NextBandPtr
)
{
    while (Band)
    {
        if (MinX >= MaxX(Band))
        {
            struct RegionRectangle *OldRect = Band;
            ADVANCE(NextBandPtr, Band);

            if (OldRect->Prev)
		OldRect->Prev->Next = Band;

  	    if (Band)
                Band->Prev = OldRect->Prev;

            DisposeRegionRectangle(OldRect);


            continue;
	}

        if (MinX > MinX(Band))
        {
            MinX(Band) = MinX;
        }
	else
        if (MaxX < MinX(Band))
        {
            struct RegionRectangle *LastRect = Band->Prev;

            DisposeBand(Band, NextBandPtr);

            if (LastRect)
                LastRect->Next = *NextBandPtr;

            if (*NextBandPtr)
                (*NextBandPtr)->Prev = LastRect;

            break;
        }

        if (MaxX < MaxX(Band))
        {
            MaxX(Band) = MaxX;
        }

        MinY(Band) = MinY;
        MaxY(Band) = MaxY;

        ADVANCE(NextBandPtr, Band);
    }
}






BOOL _AndRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
)
{
    struct RegionRectangle *rr = Reg->RegionRectangle;
    struct Rectangle OldBounds = Reg->bounds;

    if (!(Rect && _AndRectRect(Rect, &OldBounds, &Reg->bounds)))
        Rect = NULL;

    while (rr && Rect)
    {
	if
        (
            Rect->MinY > MinY(rr) + OldBounds.MinY &&
	    Rect->MinY > MaxY(rr) + OldBounds.MinY
	)
        {
            /* Teh current band is over the rectangle, so delete it */
            DisposeBand(rr, &rr);

            Reg->RegionRectangle = rr;

            if (rr)
            {
                if (rr->Prev)
                   rr->Prev->Next = NULL;

                rr->Prev = NULL;
            }
        }
        else
	if (overlapY(*Rect, *Bounds(rr)))
        {
            ShrinkBand
            (
                rr,
                Rect->MinX - OldBounds.MinX,
                MAX(Rect->MinY - OldBounds.MinY, MinY(rr)),
                Rect->MaxX - OldBounds.MinX,
                MIN(Rect->MaxY - OldBounds.MinY, MaxY(rr)),
                &rr
	    );
        }
        else
        {
            Rect = NULL;
        }
    }

    if (rr)
    {
        if (rr->Prev)
            rr->Prev->Next = NULL;
        else
            Reg->RegionRectangle = NULL;

        DisposeRegionRectangleList(rr);
    }

    if (Rect)
        _TranslateRegionRectangles(Reg->RegionRectangle, -MinX(Reg), -MinY(Reg));

    return TRUE;

}

BOOL _AndRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
)
{
    struct Region R3;

    if
    (
        _DoOperationBandBand
        (
            _AndBandBand,
            MinX(R1),
            MinX(R2),
	    MinY(R1),
            MinY(R2),
            R1->RegionRectangle,
            R2->RegionRectangle,
            &R3.RegionRectangle,
            &R3.bounds,
            GfxBase
        )
    )
    {
        DisposeRegionRectangleList(R2->RegionRectangle);

        *R2 = R3;

        _TranslateRegionRectangles(R3.RegionRectangle, -MinX(&R3), -MinY(&R3));

        return TRUE;
    }

    return FALSE;
}

BOOL _ClearRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
)
{
    struct Region R3;

    if
    (
        _DoOperationBandBand
        (
            _ClearBandBand,
            MinX(R1),
            MinX(R2),
	    MinY(R1),
            MinY(R2),
            R1->RegionRectangle,
            R2->RegionRectangle,
            &R3.RegionRectangle,
            &R3.bounds,
            GfxBase
        )
    )
    {
        DisposeRegionRectangleList(R2->RegionRectangle);

        *R2 = R3;

        _TranslateRegionRectangles(R3.RegionRectangle, -MinX(&R3), -MinY(&R3));

        return TRUE;
    }

    return FALSE;
}

BOOL _XorRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
)
{
    struct Region R3;

    if
    (
        _DoOperationBandBand
        (
            _XorBandBand,
            MinX(R1),
            MinX(R2),
	    MinY(R1),
            MinY(R2),
            R1->RegionRectangle,
            R2->RegionRectangle,
            &R3.RegionRectangle,
            &R3.bounds,
            GfxBase
        )
    )
    {
        DisposeRegionRectangleList(R2->RegionRectangle);

        *R2 = R3;

        _TranslateRegionRectangles(R3.RegionRectangle, -MinX(&R3), -MinY(&R3));

        return TRUE;
    }

    return FALSE;
}


#if DOTEST

#undef GfxBase
#include <proto/graphics.h>

int main(void)
{
    struct Region *R1 = NewRegion();
    struct Region *R2 = NewRegion();

    _OrRectRegion(R2, &(struct Rectangle){0, 0, 20, 20}, GfxBase);
    _OrRectRegion(R2, &(struct Rectangle){22, 20, 40, 40}, GfxBase);

    _AndRectRegion(R2, &(struct Rectangle){10, 10, 30, 30}, GfxBase);
    DisposeRegion(R2);
    DisposeRegion(R1);
    return 0;
}
#endif


#if DEBUG

#define _CompareRects(r1, r2)   \
(                               \
    (r1)->MinX != (r2)->MinX || \
    (r1)->MinY != (r2)->MinY || \
    (r1)->MaxX != (r2)->MaxX || \
    (r1)->MaxY != (r2)->MaxY    \
)

BOOL _AreRegionsEqual
(
    struct Region *R1,
    struct Region *R2
)
{
    struct RegionRectangle *rr1, *rr2;

    if (R1 == R2) return TRUE;

    if (_CompareRects(Bounds(R1), Bounds(R2))) return FALSE;

    for (rr1 = R1->RegionRectangle, rr2 = R2->RegionRectangle; rr1 && rr2; rr1 = rr1->Next, rr2 = rr2->Next)
    {
        if (_CompareRects(Bounds(rr1), Bounds(rr2)))
            break;
    }

    return (rr1 == NULL && rr2 == NULL);
}

void _NormalizeRegion
(
    struct Region  *R,
    struct GfxBase *GfxBase
)
{
    struct Region *normal = NewRegion();
    struct RegionRectangle *rr;

    for (rr = R->RegionRectangle; rr; rr = rr->Next)
    {
        struct Rectangle r = *Bounds(rr);
        _TranslateRect(&r, MinX(R), MinY(R));
        kprintf("»»»»» ADDING: "); dumprect(&r);
	_OrRectRegion(normal, &r, GfxBase);
        kprintf("»»»»» RESULT: "); dumpregion(normal);
    }

    DisposeRegionRectangleList(R->RegionRectangle);
    *R = *normal;
    normal->RegionRectangle = NULL;

    DisposeRegion(normal);
}

#endif
