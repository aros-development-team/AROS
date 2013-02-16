/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code for various operations on Regions and Rectangles
    Lang: English
*/

#include <exec/memory.h>
#include <graphics/regions.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <clib/macros.h>

#include "graphics_intern.h"
#include "intregions.h"

#include <aros/debug.h>

#if DEBUG
void dumplist(struct GfxBase *GfxBase)
{
    struct ChunkPool *Pool;
    int n = 0;

    Pool = (struct ChunkPool *)GetHead(&PrivGBase(GfxBase)->ChunkPoolList);

    while (Pool)
    {
        struct ChunkExt  *ChunkE;
	int m = 0;

        kprintf("Pool N.%d - %ld Chunk Free\n", n++, Pool->NumChunkFree);

        ChunkE = (struct ChunkExt *)GetHead(&Pool->ChunkList);

        while (ChunkE)
        {
            kprintf("    Chunk N.%d\n", m++);
            ChunkE = (struct ChunkExt *)GetSucc(ChunkE);
        }

        Pool = (struct ChunkPool *)GetSucc(Pool);
    }
}
#endif

static inline struct RegionRectangleExtChunk *__NewRegionRectangleExtChunk
(
    struct GfxBase *GfxBase
)
{
    struct ChunkPool *Pool;
    struct ChunkExt  *ChunkE;

    ObtainSemaphore(&PrivGBase(GfxBase)->regionsem);

    Pool = (struct ChunkPool *)GetHead(&PrivGBase(GfxBase)->ChunkPoolList);

    if (!Pool || !Pool->NumChunkFree)
    {
	int i;

        Pool = AllocMem(sizeof(struct ChunkPool), MEMF_ANY);

        if (!Pool)
        {
	    ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);

            return NULL;
        }

        NEWLIST(&Pool->ChunkList);

        Pool->NumChunkFree = SIZECHUNKBUF;

        for (i = 0; i < SIZECHUNKBUF; i++)
        {
            ADDTAIL(&Pool->ChunkList, &Pool->Chunks[i]);
        }

        ADDHEAD(&PrivGBase(GfxBase)->ChunkPoolList, Pool);
    }

    ChunkE = (struct ChunkExt *)GetHead(&Pool->ChunkList);

    REMOVE(ChunkE);

    if (!--Pool->NumChunkFree)
    {
        REMOVE(Pool);
        ADDTAIL(&PrivGBase(GfxBase)->ChunkPoolList, Pool);
    }

    ChunkE->Owner = Pool;

    ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);

    return &ChunkE->Chunk;
}


void __DisposeRegionRectangleExtChunk
(
    struct RegionRectangleExtChunk *Chunk,
    struct GfxBase *GfxBase
)
{
    struct ChunkPool *Pool = ((struct ChunkExt *)Chunk)->Owner;

    ObtainSemaphore(&PrivGBase(GfxBase)->regionsem);

    REMOVE(Pool);

    if (++Pool->NumChunkFree == SIZECHUNKBUF)
    {
        FreeMem(Pool, sizeof(struct ChunkPool));
    }
    else
    {
	ADDHEAD(&PrivGBase(GfxBase)->ChunkPoolList, Pool);
        ADDTAIL(&Pool->ChunkList, Chunk);
    }

    ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);
}


struct RegionRectangle *_NewRegionRectangle
(
    struct RegionRectangle **LastRectPtr,
    struct GfxBase *GfxBase
)
{
    struct RegionRectangleExt *RRE = RRE(*LastRectPtr);

    if (!RRE)
    {
        struct RegionRectangleExtChunk *Chunk;

        Chunk = _NewRegionRectangleExtChunk();

        if (Chunk)
	{
            RRE = Chunk->Rects;
	}

        if (RRE)
        {
            RRE->Counter = 0;
            RRE->RR.Prev = NULL;
            RRE->RR.Next = NULL;

            RRE[SIZERECTBUF - 1].RR.Next = NULL; /* IMPORTANT !!! */

            Chunk->FirstChunk = Chunk;
        }
    }
    else
    if (RRE->Counter == SIZERECTBUF - 1)
    {
        struct RegionRectangleExtChunk *Chunk;

        Chunk = _NewRegionRectangleExtChunk();

        if (Chunk)
        {
            Chunk->FirstChunk = Chunk(RRE)->FirstChunk;
            RRE = Chunk->Rects;
        }
        else
	{
            RRE = NULL;
	}

        if (RRE)
        {
            RRE->Counter = 0;
            RRE->RR.Prev = *LastRectPtr;
            RRE->RR.Next = NULL;
            (*LastRectPtr)->Next = &RRE->RR;

            RRE[SIZERECTBUF - 1].RR.Next = NULL; /* IMPORTANT !!! */
        }
    }
    else
    {
	struct RegionRectangleExt *Prev = RRE++;

        RRE->RR.Next    = NULL;
        RRE->RR.Prev    = &Prev->RR;
        Prev->RR.Next   = &RRE->RR;

        RRE->Counter    = Prev->Counter + 1;
    }

    return *LastRectPtr = (struct RegionRectangle *)RRE;
}


void _DisposeRegionRectangleList
(
    struct RegionRectangle *RR,
    struct GfxBase         *GfxBase
)
{
    struct RegionRectangleExtChunk *NextChunk;

    if (!RR)
    {
	return;
    }

    if (RR->Prev)
    {
        RR->Prev->Next = NULL;
    }

    /* Is this the first rectangle in the chunk? */
    if (!Counter(RR))
    {
	/* If so then this chunk has to be deleted too */
        NextChunk = Chunk(RR);
    }
    else
    {
	/* otherwise dispose all the chunks starting from the one after this one */
        RR = &Chunk(RR)->Rects[SIZERECTBUF - 1].RR;
        NextChunk = Chunk(RR->Next);
        RR->Next = NULL;
    }

    while (NextChunk)
    {
        struct RegionRectangleExtChunk *OldChunk = NextChunk;

        NextChunk = Chunk(NextChunk->Rects[SIZERECTBUF - 1].RR.Next);

        _DisposeRegionRectangleExtChunk(OldChunk);
    }
}

/*
   Takes all the rectangles from src and linkes them with the rectangle
   to which *dstptr points. *dstptr at the end will hold the pointer
   to the LAST rectangle in the resulting list.
   If the system runs out of memory the function will deallocate any allocated
   memory and will return FALSE. TRUE, otherwise.
 */

BOOL _LinkRegionRectangleList
(
    struct RegionRectangle  *src,
    struct RegionRectangle **dstptr,
    struct GfxBase          *GfxBase
)
{
    struct RegionRectangle *prev = *dstptr;

    for (; src; src = src->Next)
    {
        struct RegionRectangle *new = _NewRegionRectangle(dstptr, GfxBase);

        if (!new)
        {
            if (prev)
	    {
                _DisposeRegionRectangleList(prev->Next, GfxBase);
	    }

            return FALSE;
        }

        new->bounds = src->bounds;
    }

    return TRUE;
}

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

#define NEWREG(prevptr, rr)                     \
{                                               \
    rr = _NewRegionRectangle(prevptr, GfxBase); \
    if (!rr)                                    \
        return FALSE;                           \
}

#define ADDRECT(minx, maxx)    \
{                              \
   struct RegionRectangle *rr; \
   NEWREG(DstPtr, rr);         \
                               \
   MinX(rr) = minx;            \
   MinY(rr) = MinY;            \
   MaxX(rr) = maxx;            \
   MaxY(rr) = MaxY;            \
}

#define ADDRECTMERGE(minx, maxx)     \
{                                    \
    if                               \
    (                                \
        !*(DstPtr) ||                \
        (MinY(*(DstPtr)) != MinY) || \
        ((minx-1) > MaxX(*DstPtr))   \
    )                                \
    {                                \
	ADDRECT((minx), (maxx));     \
    }                                \
    else                             \
    if (MaxX(*DstPtr) < maxx)        \
    {                                \
        MaxX(*DstPtr) = maxx;        \
    }                                \
}

#define DOBANDCHECKS(firstlastdst, lastdst, curdst)                        \
if (curdst != lastdst)                                                     \
{                                                                          \
    if (!lastdst)                                                          \
    {                                                                      \
        firstlastdst = &Chunk(curdst)->FirstChunk->Rects[0].RR;            \
        DstBounds->MinY = MinY(firstlastdst);                              \
        DstBounds->MinX = MinX(firstlastdst);                              \
        DstBounds->MaxX = MaxX(curdst);                                    \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        if (DstBounds->MinX > MinX(lastdst->Next))                         \
            DstBounds->MinX = MinX(lastdst->Next);                         \
        if (DstBounds->MaxX < MaxX(curdst))                                \
            DstBounds->MaxX = MaxX(curdst);                                \
                                                                           \
        if (MaxY(firstlastdst) == MinY(curdst) - 1)                        \
        {                                                                  \
            struct RegionRectangle *_one = firstlastdst;                   \
            struct RegionRectangle *_two = lastdst->Next;                  \
                                                                           \
            while (_one != lastdst->Next && _two)                          \
            {                                                              \
		if                                                         \
                (                                                          \
                    MinX(_one) == MinX(_two) &&                            \
                    MaxX(_one) == MaxX(_two)                               \
                )                                                          \
                {                                                          \
                    _one = _one->Next;                                     \
                    _two = _two->Next;                                     \
                }                                                          \
                else                                                       \
                {                                                          \
                    break;                                                 \
                }                                                          \
	    }                                                              \
                                                                           \
            if (_one == lastdst->Next && !_two)                            \
            {                                                              \
                LONG MaxY = MaxY(curdst);                                  \
                _one = firstlastdst;                                       \
                _DisposeRegionRectangleList(lastdst->Next, GfxBase);       \
                while (_one)                                               \
                {                                                          \
                    MaxY(_one) = MaxY;                                     \
                    _one = _one->Next;                                     \
                }                                                          \
                                                                           \
                curdst = lastdst;                                          \
            }                                                              \
            else                                                           \
                firstlastdst = lastdst->Next;                              \
	}                                                                  \
        else                                                               \
            firstlastdst = lastdst->Next;                                  \
    }                                                                      \
    lastdst = curdst;                                                      \
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

    for (rr = reg->RegionRectangle; rr;)
    {
        struct RegionRectangle *rr2 = rr;

	kprintf("    Band: MinY = %d - %p\n", (int)MinY(rr), rr);
        do
        {
            kprintf("\t");dumprect(Bounds(rr2));
            ADVANCE(&rr, rr2);
        } while (rr2);
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
    while (Src1 && Src2)
    {
        if (MinX(Src1) + OffX1 < MinX(Src2) + OffX2)
        {
	    ADDRECTMERGE(MinX(Src1) + OffX1, MaxX(Src1) + OffX1);
            ADVANCE(NextSrc1Ptr, Src1);
        }
        else
        {
	    ADDRECTMERGE(MinX(Src2) + OffX2, MaxX(Src2) + OffX2);
            ADVANCE(NextSrc2Ptr, Src2);
        }
    }

    if (Src1)
    {
        do
        {
            ADDRECTMERGE(MinX(Src1) + OffX1, MaxX(Src1) + OffX1);
            ADVANCE(NextSrc1Ptr, Src1);
        } while (Src1);
    }
    else
    while (Src2)
    {
	ADDRECTMERGE(MinX(Src2) + OffX2, MaxX(Src2) + OffX2);
        ADVANCE(NextSrc2Ptr, Src2);
    }

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
    while (Src1 && Src2)
    {
        if (MinX(Src1) + OffX1 < MinX(Src2) + OffX2)
        {
	    if (MaxX(Src1) + OffX1 >= MaxX(Src2) + OffX2)
            {
            	/* Src1 totally covers Src2 */
                ADDRECT(MinX(Src2) + OffX2, MaxX(Src2) + OffX2);
 	        ADVANCE(NextSrc2Ptr, Src2);
            }
            else
            {
                if (MaxX(Src1) + OffX1 >= MinX(Src2) + OffX2)
            	    /* Src1 partially covers Src2 */
                    ADDRECT(MinX(Src2) + OffX2, MaxX(Src1) + OffX1);

                ADVANCE(NextSrc1Ptr, Src1);
            }
        }
	else
        {
	    if (MaxX(Src2) + OffX2 >= MaxX(Src1) + OffX1)
            {
            	/* Src2 totally covers Src1 */
                ADDRECT(MinX(Src1) + OffX1, MaxX(Src1) + OffX1);
 	        ADVANCE(NextSrc1Ptr, Src1);
            }
            else
            {
                if (MaxX(Src2) + OffX2 >= MinX(Src1) + OffX1)
            	    /* Src2 partially covers Src1 */
                    ADDRECT(MinX(Src1) + OffX1, MaxX(Src2) + OffX2);

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

            ADDRECT(MinX, MinX(Src1) + OffX1 - 1);

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
                 ADDRECT(MinX, MaxX(Src2) + OffX2);
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
	ADDRECT(MinX, MaxX(Src2) + OffX2);
        ADVANCE(NextSrc2Ptr, Src2);
        if (Src2)
        {
            MinX = MinX(Src2) + OffX2;
        }
    }

    return TRUE;
}


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
)
{
    struct RegionRectangle  *Dst, *LastDst, *FirstLastDst;
    struct RegionRectangle **NextSrc1Ptr = (void *)~0;
    struct RegionRectangle **NextSrc2Ptr = (void *)~0;
    struct RegionRectangle  *Band1 = Src1, *Band2 = Src2;

    BOOL res = TRUE;

    LONG TopY1 = 0, TopY2 = 0;

    FirstLastDst = LastDst = Dst = *DstPtr = NULL;

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

        if
        (
            !Operation
            (
                OffX1, OffX2,
                MinY, MaxY,
                Band1, Band2,
                &Dst,
                NextSrc1Ptr, NextSrc2Ptr,
                GfxBase
	    )
        )
        {
            res = FALSE;
            goto end;
        }

	DOBANDCHECKS(FirstLastDst, LastDst, Dst);
    }


    while (Src1)
    {
	if (NextSrc1Ptr)
            TopY1 = MinY(Src1) + OffY1;

        NextSrc1Ptr = (void *)~0;


        if
        (
            !Operation
            (
                OffX1, OffX2,
                TopY1, MaxY(Src1) + OffY1,
                Src1, NULL,
                &Dst,
                &Src1, NULL,
                GfxBase
            )
        )
        {
            res = FALSE;
            goto end;
        }

	DOBANDCHECKS(FirstLastDst, LastDst, Dst);
    }

    while (Src2)
    {
	if (NextSrc2Ptr)
            TopY2 = MinY(Src2) + OffY2;

        NextSrc2Ptr = (void *)~0;

        if
        (
            !Operation
            (
                OffX1, OffX2,
                TopY2, MaxY(Src2) + OffY2,
                NULL, Src2,
                &Dst,
                NULL, &Src2,
                GfxBase
            )
        )
        {
            res = FALSE;
            goto end;
        }

	DOBANDCHECKS(FirstLastDst, LastDst, Dst);
    }

end:

    if (Dst)
    {
        if (res)
        {
            DstBounds->MaxY = MaxY(Dst);
            *DstPtr = &Chunk(Dst)->FirstChunk->Rects[0].RR;
        }
        else
            _DisposeRegionRectangleList(&Chunk(Dst)->FirstChunk->Rects[0].RR, GfxBase);
    }

    return res;
}


#if DOTEST

#undef GfxBase
#include <proto/graphics.h>


int main(void)
{
    int i;

    struct Region *R1 = NewRegion();
    struct Region *R2 = NewRegion();

    for (i = 0; i < 10; i++)
    {
        int l = i*20;

	struct Rectangle r = {l, 0, l+11, 201};
        OrRectRegion(R1, &r);
    }

    for (i = 0; i < 10; i++)
    {
        int u = i*20;

	struct Rectangle r = {0, u, 201, u+11};
        OrRectRegion(R2, &r);
    }

    for (i = 0; i<100000; i++)
    {
        XorRegionRegion(R2, R1);
    }

    DisposeRegion(R2);
    DisposeRegion(R1);
    return 0;
}

#endif

