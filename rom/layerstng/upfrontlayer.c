/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <layers_intern.h>
#include <aros/libcall.h>
#include <proto/graphics.h>


/*****************************************************************************

    NAME */

	AROS_LH2(LONG, UpfrontLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, L, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 8, Layers)

/*  FUNCTION
        Brings a layer to the front. If this layer is a backdrop layer
        it is brought in front of all backdrop layers and behind the
        last non-backdrop layer. By clearing the BACKDROP flag of a layer
        a backdrop layer can be brought in front of all other layers.
        Parts of a simple layer that become visible are added to the 
        damage list and the REFRESH flag is set.

    INPUTS
        dummy - unused
        L     - pointer to layer

    RESULT
        TRUE  - layer was moved
        FALSE - layer could not be moved (probably out of memory)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CreateUpfrontLayer() CreateUpfrontHookLayer() BehindLayer()
        CreateBehindLayer() CreateBehindHookLayer()

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
} /* UpfrontLayer */
