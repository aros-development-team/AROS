/*
    (C) 1997 AROS - The Amiga Research OS
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

	AROS_LH2(void, EndUpdate,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, layer, A0),
	AROS_LHA(ULONG         , flag, D0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 14, Layers)

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

    D(bug("EndUpdate(layer @ $%lx)\n", layer));

    AROS_LIBFUNC_EXIT
} /* EndUpdate */
