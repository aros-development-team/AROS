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

	AROS_LH2(void, LockLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, layer, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 16, Layers)

/*  FUNCTION
        Locks a layer for exclusive access by this task.
        A layer can be locked multiple times but has to be unlocked
        as many times as it has been locked so that other tasks
        can access it.
 
    INPUTS
        layer - pointer to layer to be locked

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

    D(bug("LockLayer(layer @ $%lx)\n", layer));

    ObtainSemaphore(&layer->Lock);

    AROS_LIBFUNC_EXIT
} /* LockLayer */
