/*
    (C) 1995-97 AROS - The Amiga Research OS
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



/* Return the intersection area of a and b in intersect.
 * Return value is TRUE if a and b have such an area,  
 * else FALSE - the coordinates in intersect are not     
 * changed in this case.
 */
BOOL andrectrect(struct Rectangle* a, struct Rectangle* b, struct Rectangle* intersect)
{
    if (a->MinX <= b->MaxX) {
	if (a->MinY <= b->MaxY) {
	  if (a->MaxX >= b->MinX) {
		if (a->MaxY >= b->MinY) {
		    intersect->MinX = MAX(a->MinX, b->MinX);
		    intersect->MinY = MAX(a->MinY, b->MinY);
		    intersect->MaxX = MIN(a->MaxX, b->MaxX);
		    intersect->MaxY = MIN(a->MaxY, b->MaxY);
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
} /* andrectrect() */



/* free all memory allocated by the RegionRectangles
 * linked to rr, including rr
 */
void disposerrects(struct RegionRectangle* rr)
{
    struct RegionRectangle* rr2;

    for (; rr; rr = rr2) {
	rr2 = rr->Next;
	FreeMem(rr, sizeof(struct RegionRectangle));
    }
}  /* disposerects() */



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
	    first = AllocMem(sizeof(struct RegionRectangle), MEMF_ANY|MEMF_CLEAR);
	    if (!first)
		return FALSE;
	    first->bounds.MinX = rect->MinX;
	    first->bounds.MinY = rect->MinY;
	    first->bounds.MaxX = rect->MaxX;
	    first->bounds.MaxY = clearrect->MinY - 1;
	}
	if (rect->MaxY > clearrect->MaxY) {  /* lower */
	    new = AllocMem(sizeof(struct RegionRectangle), MEMF_ANY|MEMF_CLEAR);
	    if (!new && first) {
		disposerrects(first);
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
	    new = AllocMem(sizeof(struct RegionRectangle), MEMF_ANY|MEMF_CLEAR);
	    if (!new && first) {
		disposerrects(first);
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
	    new = AllocMem(sizeof(struct RegionRectangle), MEMF_ANY|MEMF_CLEAR);
	    if (!new && first) {
		disposerrects(first);
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
      first = AllocMem(sizeof(struct RegionRectangle), MEMF_ANY|MEMF_CLEAR);
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

    nrects = AllocMem(sizeof(struct RegionRectangle), MEMF_ANY);
    if (!nrects)
	return NULL;
    nrects->Prev = NULL;
    nrects->bounds = src->bounds;

    last = nrects;

    for (rr = src->Next; rr; rr = rr->Next) {
	if (!(cur = AllocMem(sizeof(struct RegionRectangle), MEMF_ANY))) {
	    disposerrects(nrects);
	    return NULL;
	}
	cur->bounds = rr->bounds;
	last->Next = cur;
	cur->Prev = last;
	last = cur;
    }
    last->Next = NULL;

    return nrects;
}  /* copyrrects() */



/* return a pointer to a copy of the given Region
 */
struct Region* copyregion(struct Region* r, struct GfxBase* GfxBase)
{
    struct Region* nreg;
    if ((nreg = NewRegion())) {
	nreg->bounds = r->bounds;
	if (r->RegionRectangle) {
	    if ((nreg->RegionRectangle = copyrrects(r->RegionRectangle))) {
		return nreg;
	    }
	    DisposeRegion(nreg);
	} else {
	    return nreg;
	}
    }
    return NULL;
}  /* copyregion() */



/* clear Region r2 from Region r1
 * return FALSE if not enough memory was available, else TRUE
 */
BOOL clearregionregion(struct Region* r1, struct Region* r2, struct GfxBase* GfxBase)
{
    if (r1->RegionRectangle && r2->RegionRectangle &&
	overlap(r1->bounds, r2->bounds)) {

	struct RegionRectangle* rr;
	struct RegionRectangle* backup;
	struct Rectangle clearrect;

	if (!(backup = copyrrects(r1->RegionRectangle)))
	    return FALSE;

	for (rr = r2->RegionRectangle; rr; rr = rr->Next) {
            clearrect.MinX = rr->bounds.MinX + r2->bounds.MinX;
	    clearrect.MinY = rr->bounds.MinY + r2->bounds.MinY;
	    clearrect.MaxX = rr->bounds.MaxX + r2->bounds.MinX;
	    clearrect.MaxY = rr->bounds.MaxY + r2->bounds.MinY;
	    if (!ClearRectRegion(r1, &clearrect)) {
		disposerrects(r1->RegionRectangle);
		r1->RegionRectangle = backup;
		return FALSE;
	    }
	}
	/* the backup is not needed anymore in this case */
	disposerrects(backup);
    }
    return TRUE;
}







