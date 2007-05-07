/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <exec/types.h>
#include <layers_intern.h>
#include <aros/libcall.h>
#include <proto/graphics.h>
#include "basicfuncs.h"
#include "../graphics/intregions.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH2(BOOL, IsLayerHiddenBySibling,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l              , A0),
	AROS_LHA(BOOL          , check_invisible, D0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 44, Layers)

/*  FUNCTION
       Checks whether this layer is hidden by any siblings
       that are in front of it. All these siblings must have 
       the same priority as that layer.
       It can be specified whether invisible siblings are to be
       considered in the comparison.

    INPUTS
       L               - pointer to layer 
       check_invisible - whether invisible siblings are to be considered

    RESULT
       TRUE  - layer is hidden by one or more siblings
       FALSE - layer is fully visible
  
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
	AROS_LIBFUNC_INIT

	struct Layer * _l;
	struct Region * r;
	BOOL result = FALSE;

	if (NULL == l ||
	    NULL == (r = NewRegion()))
		return FALSE;

	LockLayers(l->LayerInfo);

	_l = l->front;

	while (NULL != _l) {
		/*
		 * If they differ in priority then return FALSE.
		 */
		if (_l->priority != l->priority) {
			break;
		}
		
		/*
		 * Only need to check with those layers that
		 * have the same nesting count (are immediate
		 * siblings to the layer l).
		 */
		if (l->nesting == _l->nesting &&
		    ( IS_VISIBLE(_l) || TRUE == check_invisible) &&
		    TRUE == overlap(_l->visibleshape->bounds, l->visibleshape->bounds))
		{
			/* The layers overlap if an AND operation on
			 * both layers' visible region does not
			 * leave an empty region.
			 */
			SetRegion(l->visibleshape,r);
			AndRegionRegion(_l->visibleshape,r);
			if (NULL != r->RegionRectangle) {
				result = TRUE;
				break;
			}
		}
		_l = _l->front;
	}

	UnlockLayers(l->LayerInfo);

	DisposeRegion(r);

	return result;

	AROS_LIBFUNC_EXIT
} /* IsLayerHiddenBySibling */
