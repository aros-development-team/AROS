/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include "layers_intern.h"
#include "basicfuncs.h"
#include <exec/types.h>

/*****************************************************************************

    NAME */
#include <proto/layers.h>

	AROS_LH3(void, DoHookClipRects,

/*  SYNOPSIS */
	AROS_LHA(struct Hook      *, hook ,  A0),
	AROS_LHA(struct RastPort  *, rport,  A1),
	AROS_LHA(struct Rectangle *, rect ,  A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 36, Layers)

/*  FUNCTION

    INPUTS
        hook  - pointer to the hook to be called for the cliprects of
                the given layer
               
        rport - pointer to the rastport where the layers upon which the
                hook is to be called
        
        rect  - no operation is allowed outside this rectangle. If a layer
                is bigger than this rectangle only operations in the
                common area are allowed.

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
{ /* sv */
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

    AROS_LIBFUNC_EXIT
} /* DoHookClipRects */







