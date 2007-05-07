/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <graphics/clip.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

AROS_UFP2(BOOL, _SLCR_CompFunc_Down,
    AROS_UFPA(struct ClipRect *, cr1, A0),
    AROS_UFPA(struct ClipRect *, cr2, A1));
AROS_UFP2(BOOL, _SLCR_CompFunc_Up,
    AROS_UFPA(struct ClipRect *, cr1, A0),
    AROS_UFPA(struct ClipRect *, cr2, A1));
AROS_UFP2(BOOL, _SLCR_CompFunc_Right,
    AROS_UFPA(struct ClipRect *, cr1, A0),
    AROS_UFPA(struct ClipRect *, cr2, A1));
AROS_UFP2(BOOL, _SLCR_CompFunc_Left,
    AROS_UFPA(struct ClipRect *, cr1, A0),
    AROS_UFPA(struct ClipRect *, cr2, A1));
AROS_UFP2(BOOL, _SLCR_CompFunc_RightDown,
    AROS_UFPA(struct ClipRect *, cr1, A0),
    AROS_UFPA(struct ClipRect *, cr2, A1));
AROS_UFP2(BOOL, _SLCR_CompFunc_RightUp,
    AROS_UFPA(struct ClipRect *, cr1, A0),
    AROS_UFPA(struct ClipRect *, cr2, A1));
AROS_UFP2(BOOL, _SLCR_CompFunc_LeftDown,
    AROS_UFPA(struct ClipRect *, cr1, A0),
    AROS_UFPA(struct ClipRect *, cr2, A1));
AROS_UFP2(BOOL, _SLCR_CompFunc_LeftUp,
    AROS_UFPA(struct ClipRect *, cr1, A0),
    AROS_UFPA(struct ClipRect *, cr2, A1));

void _SLCR_SortClipRects(struct Layer *layer,
AROS_UFP2(BOOL, (*CompFunc),
    AROS_UFPA(struct ClipRect *, cr1, A0),
    AROS_UFPA(struct ClipRect *, cr2, A1))
);

/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"

	AROS_LH3(void, SortLayerCR,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, layer, A0),
	AROS_LHA(LONG          , dx,    D0),
	AROS_LHA(LONG          , dy,    D1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 35, Layers)

/*  FUNCTION
	Sorts the list of ClipRects associated with the given layer.
	The direction of the sort is indicated by dx and dy.

    INPUTS
	layer -- the layer with the ClipRect list to sort
	dx -- the left/right ordering
	dy -- the up/down ordering

    RESULT
	The layer->ClipRect pointer now points to a sorted list of ClipRects.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Implemented as an InsertSort on a singly linked list.

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("SortLayerCR(layer @ $%lx, dx %ld, dy %ld)\n", layer, dx, dy));

    if(dy > 0)
    {
	_SLCR_SortClipRects(layer, _SLCR_CompFunc_Down);

	if (dx > 0)
	    _SLCR_SortClipRects(layer, _SLCR_CompFunc_RightDown);
	else
	if (dx < 0)
	    _SLCR_SortClipRects(layer, _SLCR_CompFunc_LeftDown);
    }
    else
    if (dy < 0)
    {
	_SLCR_SortClipRects(layer, _SLCR_CompFunc_Up);

	if (dx > 0)
	    _SLCR_SortClipRects(layer, _SLCR_CompFunc_RightUp);
	else
	if (dx < 0)
	    _SLCR_SortClipRects(layer, _SLCR_CompFunc_LeftUp);
    }
    else
    {
	if (dx > 0)
	    _SLCR_SortClipRects(layer, _SLCR_CompFunc_Right);
	else
	if (dx < 0)
	    _SLCR_SortClipRects(layer, _SLCR_CompFunc_Left);
    }

    AROS_LIBFUNC_EXIT
} /* SortLayerCR */

AROS_UFH2(BOOL, _SLCR_CompFunc_Down,
    AROS_UFHA(struct ClipRect *, cr1, A0),
    AROS_UFHA(struct ClipRect *, cr2, A1))
{
    AROS_USERFUNC_INIT
    return (BOOL)(cr1->bounds.MinY <= cr2->bounds.MinY);
    AROS_USERFUNC_EXIT
}

AROS_UFH2(BOOL, _SLCR_CompFunc_Up,
    AROS_UFHA(struct ClipRect *, cr1, A0),
    AROS_UFHA(struct ClipRect *, cr2, A1))
{
    AROS_USERFUNC_INIT
    return (BOOL)(cr1->bounds.MaxY >= cr2->bounds.MaxY);
    AROS_USERFUNC_EXIT
}

AROS_UFH2(BOOL, _SLCR_CompFunc_Right,
    AROS_UFHA(struct ClipRect *, cr1, A0),
    AROS_UFHA(struct ClipRect *, cr2, A1))
{
    AROS_USERFUNC_INIT
    return (BOOL)(cr1->bounds.MinX <= cr2->bounds.MinX);
    AROS_USERFUNC_EXIT
}

AROS_UFH2(BOOL, _SLCR_CompFunc_Left,
    AROS_UFHA(struct ClipRect *, cr1, A0),
    AROS_UFHA(struct ClipRect *, cr2, A1))
{
    AROS_USERFUNC_INIT
    return (BOOL)(cr1->bounds.MaxX >= cr2->bounds.MaxX);
    AROS_USERFUNC_EXIT
}

AROS_UFH2(BOOL, _SLCR_CompFunc_RightDown,
    AROS_UFHA(struct ClipRect *, cr1, A0),
    AROS_UFHA(struct ClipRect *, cr2, A1))
{
    AROS_USERFUNC_INIT
    if(cr1->bounds.MinY > cr2->bounds.MaxY) return 0;
    return (BOOL)(cr1->bounds.MinX <= cr2->bounds.MaxX);
    AROS_USERFUNC_EXIT
}

AROS_UFH2(BOOL, _SLCR_CompFunc_RightUp,
    AROS_UFHA(struct ClipRect *, cr1, A0),
    AROS_UFHA(struct ClipRect *, cr2, A1))
{
    AROS_USERFUNC_INIT
    if(cr1->bounds.MaxY < cr2->bounds.MinY) return 0;
    return (BOOL)(cr1->bounds.MinX <= cr2->bounds.MaxX);
    AROS_USERFUNC_EXIT
}

AROS_UFH2(BOOL, _SLCR_CompFunc_LeftDown,
    AROS_UFHA(struct ClipRect *, cr1, A0),
    AROS_UFHA(struct ClipRect *, cr2, A1))
{
    AROS_USERFUNC_INIT
    if(cr1->bounds.MinY > cr2->bounds.MaxY) return 0;
    return (BOOL)(cr1->bounds.MaxX >= cr2->bounds.MinX);
    AROS_USERFUNC_EXIT
}

AROS_UFH2(BOOL, _SLCR_CompFunc_LeftUp,
    AROS_UFHA(struct ClipRect *, cr1, A0),
    AROS_UFHA(struct ClipRect *, cr2, A1))
{
    AROS_USERFUNC_INIT
    if(cr1->bounds.MaxY < cr2->bounds.MinY) return 0;
    return (BOOL)(cr1->bounds.MaxX >= cr2->bounds.MinX);
    AROS_USERFUNC_EXIT
}

void _SLCR_SortClipRects(struct Layer *layer,
AROS_UFP2(BOOL, (*CompFunc),
    AROS_UFPA(struct ClipRect *, cr1, A0),
    AROS_UFPA(struct ClipRect *, cr2, A1))
)
{

    struct ClipRect *CurCR;
    struct ClipRect *NextCR;
    struct ClipRect **CRptr;
    struct ClipRect *FirstCR;

    if(!layer->ClipRect)
	return;

    FirstCR = NULL;

    for(CurCR = layer->ClipRect, CRptr = &FirstCR; ; )
    {
	NextCR = CurCR->Next;
	CurCR->Next = *CRptr;
	*CRptr = CurCR;

	if(!NextCR)
	    break;

	for(CurCR = NextCR, CRptr = &FirstCR; ; )
	{
	    if(!(NextCR = *CRptr))
		break;

	    if(AROS_UFC2(BOOL, (*CompFunc),
                         AROS_UFCA(struct ClipRect *, CurCR, A0),
                         AROS_UFCA(struct ClipRect *, NextCR, A1)))

		break;

	    CRptr = &NextCR->Next;
	}
    }

    layer->ClipRect = FirstCR;

    return;
}
