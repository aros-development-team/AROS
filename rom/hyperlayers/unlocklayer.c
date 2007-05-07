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

	AROS_LH1(void, UnlockLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, layer, A0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 17, Layers)

/*  FUNCTION
        Unlocks a layer for access by other tasks. A layer has
        to be unlocked as many times as it has been locked until
        another task can access it.

    INPUTS
        layer - pointer to layer to be unlocked

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

    D(bug("UnlockLayer(layer @ $%lx)\n", layer));

    ReleaseSemaphore(&layer->Lock);

    AROS_LIBFUNC_EXIT
} /* UnlockLayer */
