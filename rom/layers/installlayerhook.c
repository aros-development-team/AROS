/*
    (C) 1997 AROS - The Amiga Replacement OS
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

    struct Hook *OldHook;

    D(bug("InstallLayerHook(layer @ $%lx, hook @ $%lx)\n", layer, hook));

    if(layer == NULL)
	return NULL;

    LockLayerRom(layer);

    OldHook = layer->BackFill;

    layer->BackFill = hook;

    UnlockLayerRom(layer);

    return OldHook;

    AROS_LIBFUNC_EXIT
} /* InstallLayerHook */
