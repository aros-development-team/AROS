/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/libcall.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include "layers_intern.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>

	AROS_LH2(LONG, MoveLayerInFrontOf,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, layer_to_move, A0),
	AROS_LHA(struct Layer *, other_layer, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 28, Layers)

/*  FUNCTION
        Moves layer directly in front of another layer. Other layers
        might become visible. You cannot move a backdrop layer in front
        of a non-backdrop layer.
        If parts of a simple layer become visible these areas are added
        to the damage list.

    INPUTS
        layer_to_move - pointer to layer that is to be moved
        other_layer   - pointer to other layer that will be behind the
                        layer_to_move.

    RESULT
        TRUE  - layer was moved
        FALSE - layer could not be moved. (probably out of memory)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* MoveLayerInFrontOf */
