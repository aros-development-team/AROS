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

/* return a copy of all RegionRectangles linked with the given
 * RegionRectangle or NULL if out of memory
 */
struct RegionRectangle* copyrrects(struct RegionRectangle* src)
{
    struct RegionRectangle* nrects, *cur, *last, *rr;

    nrects = NewRegionRectangle();
    if (!nrects)
	return NULL;
    nrects->Prev = NULL;
    nrects->Next = NULL; /* !!! in case allocation fails disposerects is called and this must be valid! */
    nrects->bounds = src->bounds;

    last = nrects;

    for (rr = src->Next; rr; rr = rr->Next) {
	if (!(cur = NewRegionRectangle())) {
	    DisposeRegionRectangleList(nrects);
	    return NULL;
	}
	cur->bounds = rr->bounds;
	last->Next = cur;
	cur->Prev = last;
	cur->Next = NULL; /* !!! in case allocation fails disposerects is called and this must be valid! */
	last = cur;
    }
    last->Next = NULL;

    return nrects;
}  /* copyrrects() */

