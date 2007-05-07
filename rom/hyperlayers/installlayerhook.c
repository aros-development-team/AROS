/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <utility/hooks.h>
#include <proto/graphics.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"

	AROS_LH2(struct Hook *, InstallLayerHook,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, layer, A0),
	AROS_LHA(struct Hook  *, hook, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 33, Layers)

/*  FUNCTION
        Safely install a new backfill hook. Return the old hook.
        If hook is NULL, then the default backfill hook will be installed.

    INPUTS
        layer - layer that will get the new backfill hook
        hook  - pointer to backfill hook to be installed

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

    struct Hook *OldHook;

    D(bug("InstallLayerHook(layer @ $%lx, hook @ $%lx)\n", layer, hook));

    if(layer == NULL)
	return NULL;

    LockLayer(0, layer);

    OldHook = layer->BackFill;

    layer->BackFill = hook;

    UnlockLayer(layer);

    return OldHook;

    AROS_LIBFUNC_EXIT
} /* InstallLayerHook */
