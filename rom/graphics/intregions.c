/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Code for various operations on Regions and Rectangles
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/regions.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/exec.h>

#include <clib/macros.h>
#include "intregions.h"
#include "graphics_intern.h"

#include <aros/debug.h>

#if !USE_BANDED_FUNCTIONS

/* clears from rect the area that overlaps with clearrect
 * and returns the remaining RegionRectangles in *erg
 */
BOOL clearrectrect(struct Rectangle* clearrect, struct Rectangle* rect,
		   struct RegionRectangle** erg)
{
    struct RegionRectangle* first = NULL;
    struct RegionRectangle* new;

    if (overlap(*clearrect, *rect)) { /* overlap? */
	if (rect->MinY < clearrect->MinY) {  /* upper */
	    first = NewRegionRectangle();
	    if (!first)
		return FALSE;
	    first->bounds.MinX = rect->MinX;
	    first->bounds.MinY = rect->MinY;
	    first->bounds.MaxX = rect->MaxX;
	    first->bounds.MaxY = clearrect->MinY - 1;
	}
	if (rect->MaxY > clearrect->MaxY) {  /* lower */
	    new = NewRegionRectangle();
	    if (!new && first) {
		DisposeRegionRectangleList(first);
		return FALSE;
	    }
	    new->bounds.MinX = rect->MinX;
	    new->bounds.MinY = clearrect->MaxY + 1;
	    new->bounds.MaxX = rect->MaxX;
	    new->bounds.MaxY = rect->MaxY;
	    if (first) {
		first->Prev = new;
		new->Next = first;
	    }
	    first = new;
	}
	if (rect->MinX < clearrect->MinX) {  /* left */
	    new = NewRegionRectangle();
	    if (!new && first) {
		DisposeRegionRectangleList(first);
		return FALSE;
	    }
	    new->bounds.MinX = rect->MinX;
	    new->bounds.MinY = MAX(rect->MinY, clearrect->MinY);
	    new->bounds.MaxX = clearrect->MinX-1;
	    new->bounds.MaxY = MIN(rect->MaxY, clearrect->MaxY);
	    if (first) {
		first->Prev = new;
		new->Next = first;
	    }
	    first = new;
	}
	if (rect->MaxX > clearrect->MaxX) {  /* right */
	    new = NewRegionRectangle();
	    if (!new && first) {
		DisposeRegionRectangleList(first);
		return FALSE;
	    }
	    new->bounds.MinX = clearrect->MaxX+1;
	    new->bounds.MinY = MAX(rect->MinY, clearrect->MinY);
	    new->bounds.MaxX = rect->MaxX;
	    new->bounds.MaxY = MIN(rect->MaxY, clearrect->MaxY);
	    if (first) {
		first->Prev = new;
		new->Next = first;
	    }
	    first = new;
	}
    } else {
      /* no overlap, just take the given 'rect'
       */
      first = NewRegionRectangle();
      if (!first)
	return FALSE;
      first->bounds = *rect;
    }
    *erg = first;
    return TRUE;
}  /* clearrectrect() */
#endif

/* return a copy of all RegionRectangles linked with src
 * in *dstptr. Returns FALSE in case there's no enough memory
 */
BOOL _CopyRegionRectangleList
(
    struct RegionRectangle  *src,
    struct RegionRectangle **dstptr,
#if REGIONS_HAVE_RRPOOL
    struct MinList          **RectPoolListPtr,
#endif
    struct GfxBase          *GfxBase
)
{
    struct RegionRectangle *first = NULL;
    struct RegionRectangle *prev  = NULL;

    for (; src; src = src->Next)
    {

#if REGIONS_HAVE_RRPOOL
        struct RegionRectangle *new = NewRegionRectangle(RectPoolListPtr);
#else
        struct RegionRectangle *new = NewRegionRectangle();
#endif
        if (!new)
        {
            DisposeRegionRectangleList(*dstptr);
            return FALSE;
        }

        new->Prev = prev;
        new->Next = NULL;
        new->bounds = src->bounds;

        if (!first)
            first = new;
        else
            prev->Next = new;

        prev = new;
    }

    *dstptr = first;

    return TRUE;
}

void _DisposeRegionRectangleList
(
    struct RegionRectangle *regionrectangle,
    struct GfxBase         *GfxBase
)
{
    struct RegionRectangle *next;

    ASSERT_VALID_PTR_OR_NULL(regionrectangle);

    while(regionrectangle)
    {
    	next = regionrectangle->Next;
	DisposeRegionRectangle(regionrectangle);
	regionrectangle = next;
    }
} /* DisposeRegionRectangleList */

struct RegionRectangle *_NewRegionRectangle
(
#if REGIONS_HAVE_RRPOOL
    struct MinList **RectPoolListPtr,
#endif
    struct GfxBase *GfxBase
)
{
    struct RegionRectanglePool *Pool;
    struct RegionRectangleExt  *RRE;

#if REGIONS_HAVE_RRPOOL
    if (!*RectPoolListPtr)
    {
        ObtainSemaphore(&PrivGBase(GfxBase)->regionsem);

        *RectPoolListPtr = AllocPooled
        (
            PrivGBase(GfxBase)->regionpool,
            sizeof(struct MinList)
        );

 	if (!*RectPoolListPtr)
        {
            ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);
            return NULL;
        }
        ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);

        NEWLIST(*RectPoolListPtr);

        Pool = NULL;
    }
    else
    {
        Pool = (struct RegionRectanglePool *)GetHead(*RectPoolListPtr);
    }
#else
    ObtainSemaphore(&PrivGBase(GfxBase)->rrpoolsem);
    Pool = (struct RegionRectanglePool *)GetHead(&PrivGBase(GfxBase)->rrpoollist);
#endif

    if (!Pool || !Pool->NumFreeRects)
    {
	int i;

        ObtainSemaphore(&PrivGBase(GfxBase)->regionsem);

        Pool = AllocPooled
        (
            PrivGBase(GfxBase)->regionpool,
            sizeof(struct RegionRectanglePool)
        );

 	if (!Pool)
        {
            ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);
            #if !REGIONS_HAVE_RRPOOL
            ReleaseSemaphore(&PrivGBase(GfxBase)->rrpoolsem);
            #endif
            return NULL;
        }

        Pool->RectArray = AllocPooled
        (
            PrivGBase(GfxBase)->regionpool,
            SIZERECTBUF * sizeof(struct RegionRectangleExt)
        );

        if (!Pool->RectArray)
        {
            FreePooled
            (
                PrivGBase(GfxBase)->regionpool,
                Pool,
                sizeof(struct RegionRectanglePool)
            );

            ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);
            #if !REGIONS_HAVE_RRPOOL
            ReleaseSemaphore(&PrivGBase(GfxBase)->rrpoolsem);
            #endif
            return NULL;
        }

        ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);

        NEWLIST(&Pool->List);

        for (i = 0; i < SIZERECTBUF; i++)
        {
            ADDHEAD(&Pool->List, (struct Node *)&Pool->RectArray[i]);
 	}

        Pool->NumFreeRects = SIZERECTBUF;

#if REGIONS_HAVE_RRPOOL
        ADDHEAD(*RectPoolListPtr, (struct Node *)Pool);
#else
        ADDHEAD(&PrivGBase(GfxBase)->rrpoollist, (struct Node *)Pool);
#endif
    }

    RRE = (struct RegionRectangleExt *)GetTail(&Pool->List);
    REMOVE(RRE);

    if (--Pool->NumFreeRects == 0)
    {
        REMOVE(Pool);
#if REGIONS_HAVE_RRPOOL
        ADDTAIL(*RectPoolListPtr, Pool);
#else
        ADDTAIL(&PrivGBase(GfxBase)->rrpoollist, Pool);
#endif
    }

    RRE->Owner   = Pool;

#if !REGIONS_HAVE_RRPOOL
    ReleaseSemaphore(&PrivGBase(GfxBase)->rrpoolsem);
#endif

    RRE->RR.Prev = NULL;
    RRE->RR.Next = NULL;

    return (struct RegionRectangle *)RRE;
}

void _DisposeRegionRectangle
(
    struct RegionRectangle *RR,
    struct GfxBase         *GfxBase
)
{
    struct RegionRectangleExt  *RRE  = (struct RegionRectangleExt *)RR;
    struct RegionRectanglePool *Pool = RRE->Owner;

#if !REGIONS_HAVE_RRPOOL
    ObtainSemaphore(&PrivGBase(GfxBase)->rrpoolsem);
#endif

    if (++Pool->NumFreeRects == SIZERECTBUF)
    {
        REMOVE((struct Node *)Pool);

        ObtainSemaphore(&PrivGBase(GfxBase)->regionsem);

        FreePooled
        (
            PrivGBase(GfxBase)->regionpool,
            Pool->RectArray,
            SIZERECTBUF * sizeof(struct RegionRectangleExt)
        );

        FreePooled
        (
            PrivGBase(GfxBase)->regionpool,
            Pool,
            sizeof(struct RegionRectanglePool)
        );

        ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);
    }
    else
    {
	ADDTAIL(&Pool->List, RR);
    }

#if !REGIONS_HAVE_RRPOOL
    ReleaseSemaphore(&PrivGBase(GfxBase)->rrpoolsem);
#endif
}
