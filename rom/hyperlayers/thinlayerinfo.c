/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <graphics/layers.h>
#include "basicfuncs.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>

	AROS_LH1(void, ThinLayerInfo,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 27, Layers)

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

    D(bug("ThinLayerInfo(li @ $%lx)\n", li));

    _FreeExtLayerInfo(li, LayersBase);

    li->Flags &= ~NEWLAYERINFO_CALLED;

    AROS_LIBFUNC_EXIT
} /* ThinLayerInfo */
