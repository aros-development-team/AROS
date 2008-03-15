/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/memory.h>
#include "layers_intern.h"
#include <aros/libcall.h>
#include <proto/graphics.h>
#include "basicfuncs.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH2(BOOL, IsFrontmostLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l              , A0),
	AROS_LHA(BOOL          , check_invisible, D0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 43, Layers)

/*  FUNCTION
       Checks whether this layer is the frontmost layer within
       its priority. If this layer has children then all of
       its children will be disregarded. Comparisons are only
       done with layers that have the same 'depth' of relation-
       ship (=nesting counter) to the root layer.
       It can be specified whether invisible siblings are to be
       considered in the comparison.

    INPUTS
       L               - pointer to layer 
       check_invisible - whether invisible siblings are to be considered
       

    RESULT
       TRUE  - layer is frontmost layer within its priority
       FALSE - layer is not frontmost layer within its priority
  
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
	BOOL result = TRUE;

	if (NULL == l)
		return FALSE;

	LockLayers(l->LayerInfo);

	_l = l->front;

	while (NULL != _l) {
		/*
		 * If they differ in priority then return TRUE.
		 * I did not meet another (elegible) sibling so
		 * far.
		 */
		if (_l->priority != l->priority)
			break;
		/*
		 * If the nesting counter of one layer in front
		 * of the layer l is equal (or less) then return FALSE.
		 */
		if (_l->nesting  <= l->nesting &&
		    ( IS_VISIBLE(_l) || TRUE == check_invisible)) {
			result = FALSE;
			break;
		}
		_l = _l->front;
	}

	UnlockLayers(l->LayerInfo);

	return result;

	AROS_LIBFUNC_EXIT
} /* IsFrontmostLayer */
