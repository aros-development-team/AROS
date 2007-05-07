/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/exec.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"

	AROS_LH1(void, UnlockLayerInfo,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 23, Layers)

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

    D(bug("UnlockLayerInfo(li @ $%lx)\n", li));

    ReleaseSemaphore(&li->Lock);

    AROS_LIBFUNC_EXIT
} /* UnlockLayerInfo */
