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

	AROS_LH4(LONG, SizeLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, layer, A1),
	AROS_LHA(LONG          , dx, D0),
	AROS_LHA(LONG          , dy, D1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 11, Layers)

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

    D(bug("SizeLayer(layer @ $%lx, dx %ld, dy %ld)\n", layer, dx, dy));

    return NULL;
    AROS_LIBFUNC_EXIT
} /* SizeLayer */
