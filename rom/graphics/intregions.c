/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Code for various operations on Regions and Rectangles
    Lang: english
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/regions.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
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


/* return a copy of all RegionRectangles linked with src
 * in *dstptr. Returns FALSE in case there's no enough memory
 */
BOOL _CopyRegionRectangleList
(
    struct RegionRectangle  *src,
    struct RegionRectangle **dstptr,
    struct GfxBase          *GfxBase
)
{
    struct RegionRectangle *first = NULL;
    struct RegionRectangle *prev  = NULL;

    for (; src; src = src->Next)
    {
        struct RegionRectangle *new = NewRegionRectangle();

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
