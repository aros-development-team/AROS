/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"

	AROS_LH2(LONG, MoveLayerInFrontOf,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, layer_to_move, A0),
	AROS_LHA(struct Layer *, other_layer, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 28, Layers)

/*  FUNCTION

    INPUTS

    RESULT

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

    D(bug("MoveLayerInFrontOf(layer_to_move @ $%lx, other_layer @ $%lx)\n", layer_to_move, other_layer));

    return NULL;
    AROS_LIBFUNC_EXIT
} /* MoveLayerInFrontOf */
